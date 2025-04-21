#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uni_klee_memory_check.h"

#define UNI_LOGF(f, x... ) \
  do { \
    if (f) \
      fprintf(f, x); \
      fflush(f);     \
  } while (0)

// Begin data structures
typedef unsigned long long u64;

struct uni_klee_node {
  u64 addr;
  u64 base;
  u64 size;
  u64 value;
};

struct uni_klee_vector {
  u64 size;
  u64 capacity;
  struct uni_klee_node *data;
};

struct uni_klee_arg {
  int index;
  u64 value;
  u64 size;
  char is_ptr;
  char name[256];
};

struct uni_klee_flat_map {
  int *keys;
  struct uni_klee_arg *values;
  u64 size;
};

struct uni_klee_key_value_pair {
  u64 key;
  struct uni_klee_node value;
  struct uni_klee_hash_map *map;
  struct uni_klee_key_value_pair *next;
};

struct uni_klee_hash_map {
  u64 size;
  u64 table_size;
  struct uni_klee_key_value_pair **table;
};
// End data structures

// Hash map for storing graph data from file
static struct uni_klee_vector *uni_klee_ptr_edges = NULL;
static struct uni_klee_hash_map *uni_klee_ptr_hash_map = NULL;
static struct uni_klee_hash_map *uni_klee_base_hash_map = NULL;
static struct uni_klee_flat_map *uni_klee_start_points_map = NULL;
static struct uni_klee_hash_map *uni_klee_sym_val_map = NULL;

static struct uni_klee_vector *uni_klee_vector_create(u64 cap) {
  struct uni_klee_vector *vector = (struct uni_klee_vector *)malloc(sizeof(struct uni_klee_vector));
  vector->size = 0;
  vector->capacity = cap;
  vector->data = (struct uni_klee_node *)malloc(sizeof(struct uni_klee_node) * cap);
  memset(vector->data, 0, sizeof(struct uni_klee_node) * cap);
  return vector;
}

static void uni_klee_vector_push_back(struct uni_klee_vector *vector, struct uni_klee_node node) {
  if (vector->size >= vector->capacity) {
    vector->capacity *= 2;
    vector->data = (struct uni_klee_node *)realloc(vector->data, sizeof(struct uni_klee_node) * vector->capacity);
  }
  vector->data[vector->size++] = node;
}

static struct uni_klee_node *uni_klee_vector_get(struct uni_klee_vector *vector, u64 index) {
  if (index >= vector->size) {
    return NULL;
  }
  return &vector->data[index];
}

static struct uni_klee_node *uni_klee_vector_get_back(struct uni_klee_vector *vector) {
  if (vector->size == 0) {
    return NULL;
  }
  return &vector->data[vector->size - 1];
}

static void uni_klee_vector_pop(struct uni_klee_vector *vector) {
  if (vector->size == 0) {
    return;
  }
  vector->data[vector->size - 1] = (struct uni_klee_node){0, 0, 0, 0};
  vector->size--;
}

static u64 uni_klee_vector_size(struct uni_klee_vector *vector) {
  return vector->size;
}

static void uni_klee_vector_free(struct uni_klee_vector *vector) {
  free(vector->data);
  free(vector);
}

static struct uni_klee_hash_map *uni_klee_hash_map_create(u64 cap) {
  struct uni_klee_hash_map *hash_map = (struct uni_klee_hash_map *)malloc(sizeof(struct uni_klee_hash_map));
  hash_map->size = 0;
  hash_map->table_size = cap;
  hash_map->table = (struct uni_klee_key_value_pair **)malloc(sizeof(struct uni_klee_key_value_pair *) * cap);
  memset(hash_map->table, 0, sizeof(struct uni_klee_key_value_pair *) * cap);
  return hash_map;
}

static u64 uni_klee_hash_map_fit(u64 key, u64 table_size) {
  return key % table_size;
}

static void uni_klee_hash_map_resize(struct uni_klee_hash_map *map) {
  u64 new_table_size = map->table_size * 2;
  struct uni_klee_key_value_pair **new_table = (struct uni_klee_key_value_pair **)malloc(sizeof(struct uni_klee_key_value_pair*) * new_table_size);
  memset(new_table, 0, sizeof(struct uni_klee_key_value_pair *) * new_table_size);
  for (u64 i = 0; i < map->table_size; i++) {
    struct uni_klee_key_value_pair *pair = map->table[i];
    while (pair != NULL) {
      struct uni_klee_key_value_pair *next = pair->next;
      u64 new_index = uni_klee_hash_map_fit(pair->key, new_table_size);
      pair->next = new_table[new_index];
      new_table[new_index] = pair;
      pair = next;
    }
  }
  free(map->table);
  map->table = new_table;
  map->table_size = new_table_size;
}

static void uni_klee_hash_map_insert(struct uni_klee_hash_map *map, u64 key, struct uni_klee_node value) {
  u64 index = uni_klee_hash_map_fit(key, map->table_size);
  struct uni_klee_key_value_pair *pair = map->table[index];
  while (pair != NULL) {
    if (pair->key == key) {
      // Replace the value if already exists
      pair->value = value;
      return;
    }
    pair = pair->next;
  }
  // Insert a new pair
  struct uni_klee_key_value_pair *new_pair = (struct uni_klee_key_value_pair *)malloc(sizeof(struct uni_klee_key_value_pair));
  new_pair->key = key;
  new_pair->value = value;
  new_pair->map = NULL;
  new_pair->next = map->table[index];
  map->table[index] = new_pair;
  map->size++;
  if (map->size > map->table_size * 2) {
    uni_klee_hash_map_resize(map);
  }
}

static void uni_klee_hash_map_insert_base(struct uni_klee_hash_map *map, u64 key, struct uni_klee_node value) {
  u64 index = uni_klee_hash_map_fit(key, map->table_size);
  struct uni_klee_key_value_pair *pair = map->table[index];
  while (pair != NULL) {
    if (pair->key == key) {
      if (pair->map == NULL) {
        pair->value = (struct uni_klee_node){0, 0, 0, 0};
        pair->map = uni_klee_hash_map_create(8);
      }
      UNI_LOGF(stderr, "Insert base address for the same key %llu - v %llu s %d\n", key, value.addr, pair->map->size);
      uni_klee_hash_map_insert(pair->map, value.addr, value);
      return;
    }
    pair = pair->next;
  }
  // Insert a new pair
  UNI_LOGF(stderr, "Insert base address for key %llu - v %llu\n", key, value.addr);
  struct uni_klee_key_value_pair *new_pair = (struct uni_klee_key_value_pair *)malloc(sizeof(struct uni_klee_key_value_pair));
  new_pair->key = key;
  new_pair->value = (struct uni_klee_node){0, 0, 0, 0};
  new_pair->map = uni_klee_hash_map_create(8);
  new_pair->next = map->table[index];
  map->table[index] = new_pair;
  map->size++;
  uni_klee_hash_map_insert(new_pair->map, value.addr, value);
  if (map->size > map->table_size * 2) {
    uni_klee_hash_map_resize(map);
  }
}

static struct uni_klee_key_value_pair *uni_klee_hash_map_get(struct uni_klee_hash_map *map, u64 key) {
  u64 index = uni_klee_hash_map_fit(key, map->table_size);
  struct uni_klee_key_value_pair *pair = map->table[index];
  while (pair != NULL) {
    if (pair->key == key) {
      return pair;
    }
    pair = pair->next;
  }
  return NULL;
}

static void uni_klee_hash_map_free(struct uni_klee_hash_map *map) {
  for (u64 i = 0; i < map->table_size; i++) {
    struct uni_klee_key_value_pair *pair = map->table[i];
    while (pair != NULL) {
      struct uni_klee_key_value_pair *next = pair->next;
      free(pair);
      pair = next;
    }
  }
  free(map->table);
  free(map);
}

static struct uni_klee_flat_map *uni_klee_flat_map_create(u64 size) {
  struct uni_klee_flat_map *flat_map = (struct uni_klee_flat_map *)malloc(sizeof(struct uni_klee_flat_map));
  flat_map->keys = (int *)malloc(sizeof(int) * (size + 1));
  flat_map->values = (struct uni_klee_arg *)malloc(sizeof(struct uni_klee_arg) * (size + 1));
  for (u64 i = 0; i < size; i++) {
    flat_map->keys[i] = -1;
  }
  memset(flat_map->values, 0, sizeof(struct uni_klee_arg) * (size + 1));
  flat_map->size = size;
  return flat_map;
}

static void uni_klee_flat_map_insert(struct uni_klee_flat_map *flat_map, int key, struct uni_klee_arg* value) {
  for (u64 i = 0; i < flat_map->size; i++) {
    if (flat_map->keys[i] < 0) {
      fprintf(stderr, "Inserting key %d at index %llu - value %llu size %llu is_ptr %d name %s\n", key, i, value->value, value->size, value->is_ptr, value->name);
      flat_map->keys[i] = key;
      memcpy(&flat_map->values[i], value, sizeof(struct uni_klee_arg));
      return;
    }
  }
}

static struct uni_klee_arg *uni_klee_flat_map_get(struct uni_klee_flat_map *flat_map, int key) {
  for (u64 i = 0; i < flat_map->size; i++) {
    if (flat_map->keys[i] < 0) {
      return NULL;
    }
    if (flat_map->keys[i] == key) {
      return &flat_map->values[i];
    }
  }
  return NULL;
}

static void uni_klee_initialize_hash_map(int n) {
  // Initialize the hash map
  char *base_file = getenv("UNI_KLEE_MEM_BASE_FILE");
  char *mem_file = getenv("UNI_KLEE_MEM_FILE");
  if (base_file == NULL || mem_file == NULL)
    return;
  uni_klee_ptr_edges = uni_klee_vector_create(128);
  uni_klee_ptr_hash_map = uni_klee_hash_map_create(128);
  uni_klee_base_hash_map = uni_klee_hash_map_create(128);
  uni_klee_start_points_map = uni_klee_flat_map_create(n);
  uni_klee_sym_val_map = uni_klee_hash_map_create(32);
  FILE *base_fp = fopen(base_file, "r");
  FILE *mem_fp = fopen(mem_file, "r");
  if (base_fp == NULL || mem_fp == NULL)
    return;
  size_t len = 1024;
  char line[1024];
  while (fgets(line, len, base_fp) != NULL) {
    struct uni_klee_node node = {0, 0, 0, 0};
    if(strstr(line, "[node]") != NULL) {
      sscanf(line, "[node] [addr %llu] [base %llu] [size %llu] [value %llu]",
            &node.addr, &node.base, &node.size, &node.value);
      uni_klee_hash_map_insert(uni_klee_ptr_hash_map, node.addr, node);
      uni_klee_hash_map_insert_base(uni_klee_base_hash_map, node.base, node);
    } else if(strstr(line, "[start]") != NULL) {
      struct uni_klee_arg arg = {.index=0, .value=0, .size=0, .is_ptr=1, .name={0}};
      sscanf(line, "[start] [ptr %llu] [name %[^]]] [index %d]", &arg.value, arg.name, &arg.index);
      uni_klee_flat_map_insert(uni_klee_start_points_map, arg.index, &arg);
    }
  }
  // Update memory graph for each symbolic input
  while (fgets(line, len, mem_fp) != NULL) {
    struct uni_klee_node node = {0, 0, 0, 0};
    if(strstr(line, "[node]") != NULL) {
      sscanf(line, "[node] [addr %llu] [base %llu] [size %llu] [value %llu]",
            &node.addr, &node.base, &node.size, &node.value);
      uni_klee_hash_map_insert(uni_klee_ptr_hash_map, node.addr, node);
      uni_klee_hash_map_insert_base(uni_klee_base_hash_map, node.base, node);
    } else if (strstr(line, "[sym] [arg]") != NULL) {
      struct uni_klee_arg arg = {.index=0, .value=0, .size=0, .is_ptr=0, .name={0}};
      sscanf(line, "[sym] [arg] [index %d] [size %llu] [name %[^]]]",
            &arg.index, &arg.size, arg.name);
      uni_klee_flat_map_insert(uni_klee_start_points_map, arg.index, &arg);
    } else if (strstr(line, "[sym] [heap]") != NULL) {
      char symType[16];
      char name[256];
      sscanf(line, "[sym] [heap] [type %[^]]] [addr %llu] [base %llu] [size %llu] [name %[^]]]",
            symType, &node.addr, &node.base, &node.size, name);
      // There are "heap", "sym_ptr", "lazy", "patch" - ignore sym_ptr (already handled) and patch
      if (strstr(symType, "sym_ptr") != NULL || strstr(symType, "patch") != NULL) {
        continue;
      } else {
        uni_klee_hash_map_insert(uni_klee_sym_val_map, node.addr, node);
        struct uni_klee_key_value_pair *pair = uni_klee_hash_map_get(uni_klee_sym_val_map, node.addr);
        // HACK: store the name in the map field
        pair->map = malloc(sizeof(char) * 256);
        strcpy((char *)pair->map, name);
      }
    }
  }
  // Generate the edge list
  for (u64 i = 0; i < uni_klee_ptr_hash_map->table_size; i++) {
    struct uni_klee_key_value_pair *pair = uni_klee_ptr_hash_map->table[i];
    while (pair != NULL) {
      uni_klee_vector_push_back(uni_klee_ptr_edges, pair->value);
      pair = pair->next;
    }
  }
  // Close the file
  fclose(base_fp);
  fclose(mem_fp);
}

static char *uni_klee_hex_string(char *value, u64 size) {
  char *result = malloc(2 * size + 1);
  for (u64 i = 0; i < size; i++) {
    sprintf(result + 2 * i, "%02x", (unsigned char)value[i]);
  }
  result[2 * size] = '\0';
  return result;
}

void uni_klee_heap_check(u64 *start_points, int n) {
  char *result_file = getenv("UNI_KLEE_MEM_RESULT");
  if (result_file == NULL)
    return;
  if (uni_klee_ptr_edges == NULL) {
    uni_klee_initialize_hash_map(n);
  }
  if (uni_klee_ptr_edges == NULL || uni_klee_ptr_hash_map == NULL || uni_klee_base_hash_map == NULL || uni_klee_start_points_map == NULL)
    return;
  FILE *result_fp = fopen(result_file, "a");
  if (result_fp == NULL)
    return;
  UNI_LOGF(result_fp, "[heap-check] [begin]\n");
  // Build the hash map for matching the uni-klee address to actual address
  // Hash map for matching the uni-klee address to actual address
  // First, complete the uni-klee to real address mapping && real address to uni-klee mapping
  struct uni_klee_hash_map *u2a_hash_map = uni_klee_hash_map_create(1024);
  struct uni_klee_hash_map *u2a_value_map = uni_klee_hash_map_create(1024);
  struct uni_klee_vector *u2a_queue = uni_klee_vector_create(16);
  for (int i = 0; i < n; i++) {
    void *start_point = start_points[i];
    struct uni_klee_arg *arg = uni_klee_flat_map_get(uni_klee_start_points_map, i);
    if (arg == NULL) {
      UNI_LOGF(result_fp, "[arg] [err-no-info] [index %d] [value %llu]\n", i, (u64)start_point);
      continue;
    }
    UNI_LOGF(result_fp, "[mem] [index %d] [u-addr %llu] [a-addr %llu]\n", i, (u64)arg->value, (u64)start_point);
    if (!arg->is_ptr && arg->size <= 8) {
      u64 tmp = (u64)start_point;
      char *hex = uni_klee_hex_string((char *)&tmp, arg->size);
      UNI_LOGF(result_fp, "[val] [arg] [index %d] [value %s] [size %llu] [name %s] [num %llu]\n", arg->index, hex, arg->size, arg->name, tmp);
      free(hex);
      continue;
    }
    void *value = NULL;
    if (start_point != NULL) { // Check if the pointer is NULL
      value = *(void **)start_point; // Read the value from the pointer
    }
    struct uni_klee_node actual_node = {(u64)start_point, 0, 0, (u64)value};
    struct uni_klee_key_value_pair *kv_pair = uni_klee_hash_map_get(u2a_hash_map, arg->value);
    if (kv_pair != NULL) {
      // Address already treated
      continue;
    }
    fprintf(stderr, "[u2a-map] [insert] [u-val %llu] [a-addr %llu] [a-val %llu]\n", arg->value, (u64)start_point, (u64)value);
    uni_klee_hash_map_insert(u2a_hash_map, arg->value, actual_node);
    uni_klee_vector_push_back(u2a_queue, (struct uni_klee_node){arg->value, 0, 0, (u64)start_point});
    while (uni_klee_vector_size(u2a_queue) > 0) {
      struct uni_klee_node *node = uni_klee_vector_get_back(u2a_queue);
      u64 u_addr = node->addr;
      u64 a_addr = node->value;
      u64 u_base = 0;
      u64 u_value = 0;
      uni_klee_vector_pop(u2a_queue);
      struct uni_klee_key_value_pair *pair = uni_klee_hash_map_get(uni_klee_ptr_hash_map, u_addr);
      struct uni_klee_key_value_pair *base_pair = uni_klee_hash_map_get(uni_klee_base_hash_map, u_addr);
      char is_base = base_pair != NULL;
      if (pair) {
        u_base = pair->value.base;
        u_value = pair->value.value;
      }
      u64 u_offset = u_base == 0 ? 0 : u_addr - u_base;
      fprintf(stderr, "[base] [u-addr %llu] [u-base %llu] [u-offset %llu] [u-value %llu] [a-addr %llu]\n", u_addr, u_base, u_offset, u_value, a_addr);
      if (!is_base) {
        base_pair = uni_klee_hash_map_get(uni_klee_base_hash_map, u_base);
        fprintf(stderr, "[force-base] [u-addr %llu] [u-base %llu] [u-offset %llu] [u-value %llu] [a-addr %llu]\n", u_addr, u_base, u_offset, u_value, a_addr);
      }
      char *a_base_ptr = (char *)(a_addr);
      if (base_pair) {
        // This is base address: check all outgoing pointers in object
        u64 ubase = base_pair->key;
        for (u64 i = 0; i < base_pair->map->table_size; i++) {
          // key: address from uni-klee
          // map: outgoing pointers from key
          struct uni_klee_key_value_pair *pair = base_pair->map->table[i];
          while (pair != NULL) {
            struct uni_klee_key_value_pair *a_pair = uni_klee_hash_map_get(u2a_hash_map, pair->key);
            u64 u_value = pair->value.value;
            struct uni_klee_key_value_pair *a_val_pair = uni_klee_hash_map_get(u2a_value_map, u_value);
            if (a_pair != NULL && a_val_pair != NULL) {
              fprintf(stderr, "[base-edge-fail] [u-addr %llu] [u-offset %llu] [a-addr %llu] [a-offset %llu]\n", pair->key, pair->value.addr - ubase, a_addr, (u64)a_base_ptr);
              pair = pair->next;
              continue;
            }
            u64 offset = pair->value.addr - ubase;
            fprintf(stderr, "[base-edge] [u-addr %llu] [u-offset %llu] [a-addr %llu] [a-offset %llu]\n", pair->key, offset, a_addr, (u64)a_base_ptr);
            if (a_base_ptr == NULL) {
              UNI_LOGF(result_fp, "[mem] [error] [null-pointer] [addr %llu] [base %llu] [offset %llu]\n", pair->value.addr, ubase, offset);
              continue;
            }
            char *a_ptr = a_base_ptr + offset;
            void *a_value = *(void **)a_ptr;
            UNI_LOGF(result_fp, "[mem] [mem-edge] [u-addr %llu] [u-offset %llu] [a-addr %llu] [a-value %llu]\n", pair->key, offset, (u64)a_ptr, (u64)a_value);
            struct uni_klee_node a_node = {(u64)a_ptr, (u64)(a_ptr - offset), 0, (u64)a_value};
            uni_klee_hash_map_insert(u2a_hash_map, pair->key, a_node);
            struct uni_klee_node a_value_node = {(u64)a_value, 0, 0, (u64)a_value};
            if (pair->value.value != 0) {
              fprintf(stderr, "[u2a] [insert] [u-val %llu] [a-val %llu]\n", pair->value.value, (u64)a_value);
              uni_klee_hash_map_insert(u2a_value_map, pair->value.value, a_value_node);
            }
            if (a_value != NULL)
              uni_klee_vector_push_back(u2a_queue, (struct uni_klee_node){pair->value.value, (u64)a_ptr, 0, (u64)a_value});
            pair = pair->next;
          }
        }
      } else {
        if (pair == NULL) {
          UNI_LOGF(result_fp, "[mem] [pass] [no-node] [u-addr %llu] [a-addr %llu]\n", u_addr, a_addr);
          continue;
        }
        u64 u_offset = u_addr - u_base;
        void *a_value = *(void **)a_addr;
        struct uni_klee_node a_node = {(u64)a_addr, (u64)(a_addr - u_offset), 0, (u64)a_value};
        UNI_LOGF(result_fp, "[mem] [ptr-edge] [u-addr %llu] [u-offset %llu] [a-addr %llu] [a-value %llu]\n", u_addr, u_offset, a_addr, (u64)a_value);
        uni_klee_hash_map_insert(u2a_hash_map, u_addr, a_node);
        fprintf(stderr, "[u2a] [insert] [u-val %llu] [a-val %llu]\n", u_value, (u64)a_value);
        if (a_value != NULL)
          uni_klee_vector_push_back(u2a_queue, (struct uni_klee_node){pair->value.value, (u64)a_addr, 0, (u64)a_value});
      }
    }
  }
  UNI_LOGF(result_fp, "[heap-check] [u2a-hash-map] [size %llu]\n", u2a_hash_map->size);
  for (u64 i = 0; i < uni_klee_vector_size(uni_klee_ptr_edges); i++) {
    struct uni_klee_node *node = uni_klee_vector_get(uni_klee_ptr_edges, i);
    u64 u_addr = node->addr;
    u64 u_value = node->value;
    struct uni_klee_key_value_pair *pair = uni_klee_hash_map_get(u2a_hash_map, u_addr);
    if (pair == NULL) {
      UNI_LOGF(result_fp, "[heap-check] [error] [no-mapping] [u-addr %llu] [u-value %llu]\n", u_addr, u_value);
      continue;
    }
    u64 a_addr = pair->value.addr;
    u64 a_value = pair->value.value;
    if (u_value == 1) { // Don't care about the value
      UNI_LOGF(result_fp, "[heap-check] [ok] [u-addr %llu] [u-value %llu] [a-addr %llu] [a-value %llu]\n", u_addr, u_value, a_addr, a_value);
      continue;
    }
    struct uni_klee_key_value_pair *u_value_pair = uni_klee_hash_map_get(uni_klee_ptr_hash_map, u_value);
    struct uni_klee_key_value_pair *a_value_pair = uni_klee_hash_map_get(u2a_hash_map, u_value);
    if (u_value_pair == NULL && a_value_pair == NULL) { // Both are not pointer of pointer
      if (u_value == NULL && a_value == NULL) {
        // OK
        UNI_LOGF(result_fp, "[heap-check] [ok] [u-addr %llu] [u-value %llu] [a-addr %llu] [a-value %llu]\n", u_addr, u_value, a_addr, a_value);
      } else if (u_value != NULL && a_value != NULL) {
        // Also OK
        UNI_LOGF(result_fp, "[heap-check] [ok] [u-addr %llu] [u-value %llu] [a-addr %llu] [a-value %llu]\n", u_addr, u_value, a_addr, a_value);
      } else {
        UNI_LOGF(result_fp, "[heap-check] [error] [value-mismatch] [u-addr %llu] [u-value %llu] [a-addr %llu] [a-value %llu]\n", u_addr, u_value, a_addr, a_value);
      }
    } else if (u_value_pair != NULL && a_value_pair != NULL) {
      u64 a_value_from_uni_klee = a_value_pair->value.addr;
      if (a_value != a_value_from_uni_klee) {
        UNI_LOGF(result_fp, "[heap-check] [error] [value-mismatch] [u-addr %llu] [u-value %llu] [a-addr %llu] [a-value %llu]\n", u_addr, u_value, a_addr, a_value);
      } else {
        UNI_LOGF(result_fp, "[heap-check] [ok] [u-addr %llu] [u-value %llu] [a-addr %llu] [a-value %llu]\n", u_addr, u_value, a_addr, a_value);
      }
    } else {
      UNI_LOGF(result_fp, "[heap-check] [error] [value-mismatch] [u-addr %llu] [u-value %llu] [a-addr %llu] [a-value %llu]\n", u_addr, u_value, a_addr, a_value);
    }
  }
  // Collect values from heap
  for (u64 i = 0; i < uni_klee_sym_val_map->table_size; i++) {
    struct uni_klee_key_value_pair *pair = uni_klee_sym_val_map->table[i];
    while (pair != NULL) {
      u64 u_addr = pair->key;
      // HACK: load the name from the map field
      char *name = (char *)pair->map;
      u64 u_base = pair->value.base;
      u64 size = pair->value.size;
      pair = pair->next;
      u64 a_addr = 0;
      struct uni_klee_key_value_pair *a_pair = uni_klee_hash_map_get(u2a_value_map, u_addr);
      if (a_pair == NULL) {
        // Search using u_base
        a_pair = uni_klee_hash_map_get(u2a_hash_map, u_base);
        if (a_pair == NULL) {
          UNI_LOGF(result_fp, "[val] [error] [no-mapping] [u-addr %llu] [name %s]\n", u_addr, name);
          continue;
        }
        a_addr = a_pair->value.addr + (u_addr - u_base);
      } else {
        a_addr = a_pair->value.addr;
      }
      if (a_addr == NULL) {
        UNI_LOGF(result_fp, "[val] [error] [null-pointer] [addr %llu] [name %s]\n", u_addr, name);
        continue;
      }
      char *a_base_ptr = (char *)a_addr;
      // Print the data as hex string
      char *hex = uni_klee_hex_string(a_base_ptr, size);
      u64 value = 0;
      if (size == 1) {
        unsigned char v = *(unsigned char *)a_base_ptr;
        value = (u64)v;
      } else if (size == 2) {
        unsigned short v = *(unsigned short *)a_base_ptr;
        value = (u64)v;
      } else if (size == 4) {
        unsigned int v = *(unsigned int *)a_base_ptr;
        value = (u64)v;
      } else if (size == 8) {
        value = *(u64 *)a_base_ptr;
      } else {
        value = *(u64 *)a_base_ptr;
      }
      UNI_LOGF(result_fp, "[val] [heap] [u-addr %llu] [name %s] [value %s] [size %llu] [num %llu]\n", u_addr, name, hex, size, value);
      free(hex);
    }
  }
  UNI_LOGF(result_fp, "[heap-check] [end]\n");
  fclose(result_fp);
  uni_klee_hash_map_free(u2a_hash_map);
  uni_klee_vector_free(u2a_queue);
}