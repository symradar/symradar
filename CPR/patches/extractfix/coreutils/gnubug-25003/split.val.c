/* split.c -- split a file into pieces.
   Copyright (C) 1988-2016 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* By tege@sics.se, with rms.

   TODO:
   * support -p REGEX as in BSD's split.
   * support --suppress-matched as in csplit.  */
#include <config.h>

#include <assert.h>
#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "system.h"
#include "die.h"
#include "error.h"
#include "fd-reopen.h"
#include "fcntl--.h"
#include "full-write.h"
#include "ioblksize.h"
#include "quote.h"
#include "safe-read.h"
#include "sig2str.h"
#include "xfreopen.h"
#include "xdectoint.h"
#include "xstrtol.h"

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

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "split"

#define AUTHORS \
  proper_name ("Torbjorn Granlund"), \
  proper_name ("Richard M. Stallman")

/* Shell command to filter through, instead of creating files.  */
static char const *filter_command;

/* Process ID of the filter.  */
static int filter_pid;

/* Array of open pipes.  */
static int *open_pipes;
static size_t open_pipes_alloc;
static size_t n_open_pipes;

/* Blocked signals.  */
static sigset_t oldblocked;
static sigset_t newblocked;

/* Base name of output files.  */
static char const *outbase;

/* Name of output files.  */
static char *outfile;

/* Pointer to the end of the prefix in OUTFILE.
   Suffixes are inserted here.  */
static char *outfile_mid;

/* Generate new suffix when suffixes are exhausted.  */
static bool suffix_auto = true;

/* Length of OUTFILE's suffix.  */
static size_t suffix_length;

/* Alphabet of characters to use in suffix.  */
static char const *suffix_alphabet = "abcdefghijklmnopqrstuvwxyz";

/* Numerical suffix start value.  */
static const char *numeric_suffix_start;

/* Additional suffix to append to output file names.  */
static char const *additional_suffix;

/* Name of input file.  May be "-".  */
static char *infile;

/* stat buf for input file.  */
static struct stat in_stat_buf;

/* Descriptor on which output file is open.  */
static int output_desc = -1;

/* If true, print a diagnostic on standard error just before each
   output file is opened. */
static bool verbose;

/* If true, don't generate zero length output files. */
static bool elide_empty_files;

/* If true, in round robin mode, immediately copy
   input to output, which is much slower, so disabled by default.  */
static bool unbuffered;

/* The character marking end of line.  Defaults to \n below.  */
static int eolchar = -1;

/* The split mode to use.  */
enum Split_type
{
  type_undef, type_bytes, type_byteslines, type_lines, type_digits,
  type_chunk_bytes, type_chunk_lines, type_rr
};

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  VERBOSE_OPTION = CHAR_MAX + 1,
  FILTER_OPTION,
  IO_BLKSIZE_OPTION,
  ADDITIONAL_SUFFIX_OPTION
};

static struct option const longopts[] =
{
  {"bytes", required_argument, NULL, 'b'},
  {"lines", required_argument, NULL, 'l'},
  {"line-bytes", required_argument, NULL, 'C'},
  {"number", required_argument, NULL, 'n'},
  {"elide-empty-files", no_argument, NULL, 'e'},
  {"unbuffered", no_argument, NULL, 'u'},
  {"suffix-length", required_argument, NULL, 'a'},
  {"additional-suffix", required_argument, NULL,
   ADDITIONAL_SUFFIX_OPTION},
  {"numeric-suffixes", optional_argument, NULL, 'd'},
  {"filter", required_argument, NULL, FILTER_OPTION},
  {"verbose", no_argument, NULL, VERBOSE_OPTION},
  {"separator", required_argument, NULL, 't'},
  {"-io-blksize", required_argument, NULL,
   IO_BLKSIZE_OPTION}, /* do not document */
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Return true if the errno value, ERR, is ignorable.  */
static inline bool
ignorable (int err)
{
  return filter_command && err == EPIPE;
}

static void
set_suffix_length (uintmax_t n_units, enum Split_type split_type)
{
#define DEFAULT_SUFFIX_LENGTH 2

  uintmax_t suffix_needed = 0;

  /* The suffix auto length feature is incompatible with
     a user specified start value as the generated suffixes
     are not all consecutive.  */
  if (numeric_suffix_start)
    suffix_auto = false;

  /* Auto-calculate the suffix length if the number of files is given.  */
  if (split_type == type_chunk_bytes || split_type == type_chunk_lines
      || split_type == type_rr)
    {
      uintmax_t n_units_end = n_units;
      if (numeric_suffix_start)
        {
          uintmax_t n_start;
          strtol_error e = xstrtoumax (numeric_suffix_start, NULL, 10,
                                       &n_start, "");
          if (e == LONGINT_OK && n_start <= UINTMAX_MAX - n_units)
            {
              /* Restrict auto adjustment so we don't keep
                 incrementing a suffix size arbitrarily,
                 as that would break sort order for files
                 generated from multiple split runs.  */
              if (n_start < n_units)
                n_units_end += n_start;
            }

        }
      size_t alphabet_len = strlen (suffix_alphabet);
      bool alphabet_slop = (n_units_end % alphabet_len) != 0;
      while (n_units_end /= alphabet_len)
        suffix_needed++;
      suffix_needed += alphabet_slop;
      suffix_auto = false;
    }

  if (suffix_length)            /* set by user */
    {
      if (suffix_length < suffix_needed)
        {
          die (EXIT_FAILURE, 0,
               _("the suffix length needs to be at least %"PRIuMAX),
               suffix_needed);
        }
      suffix_auto = false;
      return;
    }
  else
    suffix_length = MAX (DEFAULT_SUFFIX_LENGTH, suffix_needed);
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE [PREFIX]]\n\
"),
              program_name);
      fputs (_("\
Output pieces of FILE to PREFIXaa, PREFIXab, ...;\n\
default size is 1000 lines, and default PREFIX is 'x'.\n\
"), stdout);

      emit_stdin_note ();
      emit_mandatory_arg_note ();

      fprintf (stdout, _("\
  -a, --suffix-length=N   generate suffixes of length N (default %d)\n\
      --additional-suffix=SUFFIX  append an additional SUFFIX to file names\n\
  -b, --bytes=SIZE        put SIZE bytes per output file\n\
  -C, --line-bytes=SIZE   put at most SIZE bytes of records per output file\n\
  -d                      use numeric suffixes starting at 0, not alphabetic\n\
      --numeric-suffixes[=FROM]  same as -d, but allow setting the start value\
\n\
  -e, --elide-empty-files  do not generate empty output files with '-n'\n\
      --filter=COMMAND    write to shell COMMAND; file name is $FILE\n\
  -l, --lines=NUMBER      put NUMBER lines/records per output file\n\
  -n, --number=CHUNKS     generate CHUNKS output files; see explanation below\n\
  -t, --separator=SEP     use SEP instead of newline as the record separator;\n\
                            '\\0' (zero) specifies the NUL character\n\
  -u, --unbuffered        immediately copy input to output with '-n r/...'\n\
"), DEFAULT_SUFFIX_LENGTH);
      fputs (_("\
      --verbose           print a diagnostic just before each\n\
                            output file is opened\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_size_note ();
      fputs (_("\n\
CHUNKS may be:\n\
  N       split into N files based on size of input\n\
  K/N     output Kth of N to stdout\n\
  l/N     split into N files without splitting lines/records\n\
  l/K/N   output Kth of N to stdout without splitting lines/records\n\
  r/N     like 'l' but use round robin distribution\n\
  r/K/N   likewise but only output Kth of N to stdout\n\
"), stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* Return the number of bytes that can be read from FD with status ST.
   Store up to the first BUFSIZE bytes of the file's data into BUF,
   and advance the file position by the number of bytes read.  On
   input error, set errno and return -1.  */

static off_t
input_file_size (int fd, struct stat const *st, char *buf, size_t bufsize)
{
  off_t cur = lseek (fd, 0, SEEK_CUR);
  if (cur < 0)
    {
      if (errno == ESPIPE)
        errno = 0; /* Suppress confusing seek error.  */
      return -1;
    }

  off_t size = 0;
  do
    {
      size_t n_read = safe_read (fd, buf + size, bufsize - size);
      if (n_read == 0)
        return size;
      if (n_read == SAFE_READ_ERROR)
        return -1;
      size += n_read;
    }
  while (size < bufsize);

  /* Note we check st_size _after_ the read() above
     because /proc files on GNU/Linux are seekable
     but have st_size == 0.  */
  if (st->st_size == 0)
    {
      /* We've filled the buffer, from a seekable file,
         which has an st_size==0, E.g., /dev/zero on GNU/Linux.
         Assume there is no limit to file size.  */
      errno = EOVERFLOW;
      return -1;
    }

  cur += size;
  off_t end;
  if (usable_st_size (st) && cur <= st->st_size)
    end = st->st_size;
  else
    {
      end = lseek (fd, 0, SEEK_END);
      if (end < 0)
        return -1;
      if (end != cur)
        {
          if (lseek (fd, cur, SEEK_SET) < 0)
            return -1;
          if (end < cur)
            end = cur;
        }
    }

  size += end - cur;
  if (size == OFF_T_MAX)
    {
      /* E.g., /dev/zero on GNU/Hurd.  */
      errno = EOVERFLOW;
      return -1;
    }

  return size;
}

/* Compute the next sequential output file name and store it into the
   string 'outfile'.  */

static void
next_file_name (void)
{
  /* Index in suffix_alphabet of each character in the suffix.  */
  static size_t *sufindex;
  static size_t outbase_length;
  static size_t outfile_length;
  static size_t addsuf_length;

  if (! outfile)
    {
      bool widen;

new_name:
      widen = !! outfile_length;

      if (! widen)
        {
          /* Allocate and initialize the first file name.  */

          outbase_length = strlen (outbase);
          addsuf_length = additional_suffix ? strlen (additional_suffix) : 0;
          outfile_length = outbase_length + suffix_length + addsuf_length;
        }
      else
        {
          /* Reallocate and initialize a new wider file name.
             We do this by subsuming the unchanging part of
             the generated suffix into the prefix (base), and
             reinitializing the now one longer suffix.  */

          outfile_length += 2;
          suffix_length++;
        }

      if (outfile_length + 1 < outbase_length)
        xalloc_die ();
      outfile = xrealloc (outfile, outfile_length + 1);

      if (! widen)
        memcpy (outfile, outbase, outbase_length);
      else
        {
          /* Append the last alphabet character to the file name prefix.  */
          outfile[outbase_length] = suffix_alphabet[sufindex[0]];
          outbase_length++;
        }

      outfile_mid = outfile + outbase_length;
      memset (outfile_mid, suffix_alphabet[0], suffix_length);
      if (additional_suffix)
        memcpy (outfile_mid + suffix_length, additional_suffix, addsuf_length);
      outfile[outfile_length] = 0;

      free (sufindex);
      sufindex = xcalloc (suffix_length, sizeof *sufindex);

      if (numeric_suffix_start)
        {
          assert (! widen);

          /* Update the output file name.  */
          size_t i = strlen (numeric_suffix_start);
          memcpy (outfile_mid + suffix_length - i, numeric_suffix_start, i);

          /* Update the suffix index.  */
          size_t *sufindex_end = sufindex + suffix_length;
          while (i-- != 0)
            *--sufindex_end = numeric_suffix_start[i] - '0';
        }

#if ! _POSIX_NO_TRUNC && HAVE_PATHCONF && defined _PC_NAME_MAX
      /* POSIX requires that if the output file name is too long for
         its directory, 'split' must fail without creating any files.
         This must be checked for explicitly on operating systems that
         silently truncate file names.  */
      {
        char *dir = dir_name (outfile);
        long name_max = pathconf (dir, _PC_NAME_MAX);
        if (0 <= name_max && name_max < base_len (last_component (outfile)))
          die (EXIT_FAILURE, ENAMETOOLONG, "%s", quotef (outfile));
        free (dir);
      }
#endif
    }
  else
    {
      /* Increment the suffix in place, if possible.  */

      size_t i = suffix_length;
      while (i-- != 0)
        {
          sufindex[i]++;
          if (suffix_auto && i == 0 && ! suffix_alphabet[sufindex[0] + 1])
            goto new_name;
          outfile_mid[i] = suffix_alphabet[sufindex[i]];
          if (outfile_mid[i])
            return;
          sufindex[i] = 0;
          outfile_mid[i] = suffix_alphabet[sufindex[i]];
        }
      die (EXIT_FAILURE, 0, _("output file suffixes exhausted"));
    }
}

/* Create or truncate a file.  */

static int
create (const char *name)
{
  if (!filter_command)
    {
      if (verbose)
        fprintf (stdout, _("creating file %s\n"), quoteaf (name));

      int fd = open (name, O_WRONLY | O_CREAT | O_BINARY, MODE_RW_UGO);
      if (fd < 0)
        return fd;
      struct stat out_stat_buf;
      if (fstat (fd, &out_stat_buf) != 0)
        die (EXIT_FAILURE, errno, _("failed to stat %s"), quoteaf (name));
      if (SAME_INODE (in_stat_buf, out_stat_buf))
        die (EXIT_FAILURE, 0, _("%s would overwrite input; aborting"),
             quoteaf (name));
      if (ftruncate (fd, 0) != 0)
        die (EXIT_FAILURE, errno, _("%s: error truncating"), quotef (name));

      return fd;
    }
  else
    {
      int fd_pair[2];
      pid_t child_pid;
      char const *shell_prog = getenv ("SHELL");
      if (shell_prog == NULL)
        shell_prog = "/bin/sh";
      if (setenv ("FILE", name, 1) != 0)
        die (EXIT_FAILURE, errno,
             _("failed to set FILE environment variable"));
      if (verbose)
        fprintf (stdout, _("executing with FILE=%s\n"), quotef (name));
      if (pipe (fd_pair) != 0)
        die (EXIT_FAILURE, errno, _("failed to create pipe"));
      child_pid = fork ();
      if (child_pid == 0)
        {
          /* This is the child process.  If an error occurs here, the
             parent will eventually learn about it after doing a wait,
             at which time it will emit its own error message.  */
          int j;
          /* We have to close any pipes that were opened during an
             earlier call, otherwise this process will be holding a
             write-pipe that will prevent the earlier process from
             reading an EOF on the corresponding read-pipe.  */
          for (j = 0; j < n_open_pipes; ++j)
            if (close (open_pipes[j]) != 0)
              die (EXIT_FAILURE, errno, _("closing prior pipe"));
          if (close (fd_pair[1]))
            die (EXIT_FAILURE, errno, _("closing output pipe"));
          if (fd_pair[0] != STDIN_FILENO)
            {
              if (dup2 (fd_pair[0], STDIN_FILENO) != STDIN_FILENO)
                die (EXIT_FAILURE, errno, _("moving input pipe"));
              if (close (fd_pair[0]) != 0)
                die (EXIT_FAILURE, errno, _("closing input pipe"));
            }
          sigprocmask (SIG_SETMASK, &oldblocked, NULL);
          execl (shell_prog, last_component (shell_prog), "-c",
                 filter_command, (char *) NULL);
          die (EXIT_FAILURE, errno, _("failed to run command: \"%s -c %s\""),
               shell_prog, filter_command);
        }
      if (child_pid == -1)
        die (EXIT_FAILURE, errno, _("fork system call failed"));
      if (close (fd_pair[0]) != 0)
        die (EXIT_FAILURE, errno, _("failed to close input pipe"));
      filter_pid = child_pid;
      if (n_open_pipes == open_pipes_alloc)
        open_pipes = x2nrealloc (open_pipes, &open_pipes_alloc,
                                 sizeof *open_pipes);
      open_pipes[n_open_pipes++] = fd_pair[1];
      return fd_pair[1];
    }
}

/* Close the output file, and do any associated cleanup.
   If FP and FD are both specified, they refer to the same open file;
   in this case FP is closed, but FD is still used in cleanup.  */
static void
closeout (FILE *fp, int fd, pid_t pid, char const *name)
{
  if (fp != NULL && fclose (fp) != 0 && ! ignorable (errno))
    die (EXIT_FAILURE, errno, "%s", quotef (name));
  if (fd >= 0)
    {
      if (fp == NULL && close (fd) < 0)
        die (EXIT_FAILURE, errno, "%s", quotef (name));
      int j;
      for (j = 0; j < n_open_pipes; ++j)
        {
          if (open_pipes[j] == fd)
            {
              open_pipes[j] = open_pipes[--n_open_pipes];
              break;
            }
        }
    }
  if (pid > 0)
    {
      int wstatus = 0;
      if (waitpid (pid, &wstatus, 0) == -1 && errno != ECHILD)
        die (EXIT_FAILURE, errno, _("waiting for child process"));
      if (WIFSIGNALED (wstatus))
        {
          int sig = WTERMSIG (wstatus);
          if (sig != SIGPIPE)
            {
              char signame[MAX (SIG2STR_MAX, INT_BUFSIZE_BOUND (int))];
              if (sig2str (sig, signame) != 0)
                sprintf (signame, "%d", sig);
              error (sig + 128, 0,
                     _("with FILE=%s, signal %s from command: %s"),
                     quotef (name), signame, filter_command);
            }
        }
      else if (WIFEXITED (wstatus))
        {
          int ex = WEXITSTATUS (wstatus);
          if (ex != 0)
            error (ex, 0, _("with FILE=%s, exit %d from command: %s"),
                   quotef (name), ex, filter_command);
        }
      else
        {
          /* shouldn't happen.  */
          die (EXIT_FAILURE, 0,
               _("unknown status from command (0x%X)"), wstatus + 0u);
        }
    }
}

/* Write BYTES bytes at BP to an output file.
   If NEW_FILE_FLAG is true, open the next output file.
   Otherwise add to the same output file already in use.
   Return true if successful.  */

static bool
cwrite (bool new_file_flag, const char *bp, size_t bytes)
{
return false;
  if (new_file_flag)
    {
      if (!bp && bytes == 0 && elide_empty_files)
        return true;
      closeout (NULL, output_desc, filter_pid, outfile);
      next_file_name ();
      output_desc = create (outfile);
      if (output_desc < 0)
        die (EXIT_FAILURE, errno, "%s", quotef (outfile));
    }

  if (full_write (output_desc, bp, bytes) == bytes)
    return true;
  else
    {
      if (! ignorable (errno))
        die (EXIT_FAILURE, errno, "%s", quotef (outfile));
      return false;
    }
}

/* Split into pieces of exactly N_BYTES bytes.
   Use buffer BUF, whose size is BUFSIZE.
   BUF contains the first INITIAL_READ input bytes.  */

static void
bytes_split (uintmax_t n_bytes, char *buf, size_t bufsize, size_t initial_read,
             uintmax_t max_files)
{
  size_t n_read;
  bool new_file_flag = true;
  uintmax_t to_write = n_bytes;
  uintmax_t opened = 0;
  bool eof;

  do
    {
      if (initial_read != SIZE_MAX)
        {
          n_read = initial_read;
          initial_read = SIZE_MAX;
          eof = n_read < bufsize;
        }
      else
        {
          n_read = safe_read (STDIN_FILENO, buf, bufsize);
          if (n_read == SAFE_READ_ERROR)
            die (EXIT_FAILURE, errno, "%s", quotef (infile));
          eof = n_read == 0;
        }
      char *bp_out = buf;
      size_t to_read = n_read;
      while (to_write <= to_read)
        {
          size_t w = to_write;
          bool cwrite_ok = cwrite (new_file_flag, bp_out, w);
          opened += new_file_flag;
          new_file_flag = !max_files || (opened < max_files);
          if (!new_file_flag && !cwrite_ok)
            {
              /* If filter no longer accepting input, stop reading.  */
              n_read = to_read = 0;
              break;
            }
          bp_out += w;
          to_read -= w;
          to_write = n_bytes;
        }
      if (to_read != 0)
        {
          bool cwrite_ok = cwrite (new_file_flag, bp_out, to_read);
          opened += new_file_flag;
          to_write -= to_read;
          new_file_flag = false;
          if (!cwrite_ok)
            {
              /* If filter no longer accepting input, stop reading.  */
              n_read = 0;
              break;
            }
        }
    }
  while (! eof);

  /* Ensure NUMBER files are created, which truncates
     any existing files or notifies any consumers on fifos.
     FIXME: Should we do this before EXIT_FAILURE?  */
  while (opened++ < max_files)
    cwrite (true, NULL, 0);
}

/* Split into pieces of exactly N_LINES lines.
   Use buffer BUF, whose size is BUFSIZE.  */

static void
lines_split (uintmax_t n_lines, char *buf, size_t bufsize)
{
  size_t n_read;
  char *bp, *bp_out, *eob;
  bool new_file_flag = true;
  uintmax_t n = 0;

  do
    {
      n_read = safe_read (STDIN_FILENO, buf, bufsize);
      if (n_read == SAFE_READ_ERROR)
        die (EXIT_FAILURE, errno, "%s", quotef (infile));
      bp = bp_out = buf;
      eob = bp + n_read;
      *eob = eolchar;
      while (true)
        {
          bp = memchr (bp, eolchar, eob - bp + 1);
          if (bp == eob)
            {
              if (eob != bp_out) /* do not write 0 bytes! */
                {
                  size_t len = eob - bp_out;
                  cwrite (new_file_flag, bp_out, len);
                  new_file_flag = false;
                }
              break;
            }

          ++bp;
          if (++n >= n_lines)
            {
              cwrite (new_file_flag, bp_out, bp - bp_out);
              bp_out = bp;
              new_file_flag = true;
              n = 0;
            }
        }
    }
  while (n_read);
}

/* Split into pieces that are as large as possible while still not more
   than N_BYTES bytes, and are split on line boundaries except
   where lines longer than N_BYTES bytes occur. */

static void
line_bytes_split (uintmax_t n_bytes, char *buf, size_t bufsize)
{
  size_t n_read;
  uintmax_t n_out = 0;      /* for each split.  */
  size_t n_hold = 0;
  char *hold = NULL;        /* for lines > bufsize.  */
  size_t hold_size = 0;
  bool split_line = false;  /* Whether a \n was output in a split.  */

  do
    {
      n_read = safe_read (STDIN_FILENO, buf, bufsize);
      if (n_read == SAFE_READ_ERROR)
        die (EXIT_FAILURE, errno, "%s", quotef (infile));
      size_t n_left = n_read;
      char *sob = buf;
      while (n_left)
        {
          size_t split_rest = 0;
          char *eoc = NULL;
          char *eol;

          /* Determine End Of Chunk and/or End of Line,
             which are used below to select what to write or buffer.  */
          if (n_bytes - n_out - n_hold <= n_left)
            {
              /* Have enough for split.  */
              split_rest = n_bytes - n_out - n_hold;
              eoc = sob + split_rest - 1;
              eol = memrchr (sob, eolchar, split_rest);
            }
          else
            eol = memrchr (sob, eolchar, n_left);

          /* Output hold space if possible.  */
          if (n_hold && !(!eol && n_out))
            {
              cwrite (n_out == 0, hold, n_hold);
              n_out += n_hold;
              if (n_hold > bufsize)
                hold = xrealloc (hold, bufsize);
              n_hold = 0;
              hold_size = bufsize;
            }

          /* Output to eol if present.  */
          if (eol)
            {
              split_line = true;
              size_t n_write = eol - sob + 1;
              cwrite (n_out == 0, sob, n_write);
              n_out += n_write;
              n_left -= n_write;
              sob += n_write;
              if (eoc)
                split_rest -= n_write;
            }

          /* Output to eoc or eob if possible.  */
          if (n_left && !split_line)
            {
              size_t n_write = eoc ? split_rest : n_left;
              cwrite (n_out == 0, sob, n_write);
              n_out += n_write;
              n_left -= n_write;
              sob += n_write;
              if (eoc)
                split_rest -= n_write;
            }

          /* Update hold if needed.  */
          if ((eoc && split_rest) || (!eoc && n_left))
            {
              size_t n_buf = eoc ? split_rest : n_left;
              if (hold_size - n_hold < n_buf)
                {
                  if (hold_size <= SIZE_MAX - bufsize)
                    hold_size += bufsize;
                  else
                    xalloc_die ();
                  hold = xrealloc (hold, hold_size);
                }
              memcpy (hold + n_hold, sob, n_buf);
              n_hold += n_buf;
              n_left -= n_buf;
              sob += n_buf;
            }

          /* Reset for new split.  */
          if (eoc)
            {
              n_out = 0;
              split_line = false;
            }
        }
    }
  while (n_read);

  /* Handle no eol at end of file.  */
  if (n_hold)
    cwrite (n_out == 0, hold, n_hold);

  free (hold);
}

/* -n l/[K/]N: Write lines to files of approximately file size / N.
   The file is partitioned into file size / N sized portions, with the
   last assigned any excess.  If a line _starts_ within a partition
   it is written completely to the corresponding file.  Since lines
   are not split even if they overlap a partition, the files written
   can be larger or smaller than the partition size, and even empty
   if a line is so long as to completely overlap the partition.  */

static void
lines_chunk_split (uintmax_t k, uintmax_t n, char *buf, size_t bufsize,
                   size_t initial_read, off_t file_size)
{
  assert (n && k <= n && n <= file_size);

  const off_t chunk_size = file_size / n;
  uintmax_t chunk_no = 1;
  off_t chunk_end = chunk_size - 1;
  off_t n_written = 0;
  bool new_file_flag = true;
  bool chunk_truncated = false;

  if (k > 1)
    {
      /* Start reading 1 byte before kth chunk of file.  */
      off_t start = (k - 1) * chunk_size - 1;
      if (start < initial_read)
        {
          memmove (buf, buf + start, initial_read - start);
          initial_read -= start;
        }
      else
        {
          if (lseek (STDIN_FILENO, start - initial_read, SEEK_CUR) < 0)
            die (EXIT_FAILURE, errno, "%s", quotef (infile));
          initial_read = SIZE_MAX;
        }
      n_written = start;
      chunk_no = k - 1;
      chunk_end = chunk_no * chunk_size - 1;
    }

  while (n_written < file_size)
    {
      char *bp = buf, *eob;
      size_t n_read;
      if (initial_read != SIZE_MAX)
        {
          n_read = initial_read;
          initial_read = SIZE_MAX;
        }
      else
        {
          n_read = safe_read (STDIN_FILENO, buf, bufsize);
          if (n_read == SAFE_READ_ERROR)
            die (EXIT_FAILURE, errno, "%s", quotef (infile));
        }
      if (n_read == 0)
        break; /* eof.  */
      n_read = MIN (n_read, file_size - n_written);
      chunk_truncated = false;
      eob = buf + n_read;

      while (bp != eob)
        {
          size_t to_write;
          bool next = false;

          /* Begin looking for '\n' at last byte of chunk.  */
          off_t skip = MIN (n_read, MAX (0, chunk_end - n_written));
          char *bp_out = memchr (bp + skip, eolchar, n_read - skip);
          if (bp_out++)
            next = true;
          else
            bp_out = eob;
          to_write = bp_out - bp;

          if (k == chunk_no)
            {
              /* We don't use the stdout buffer here since we're writing
                 large chunks from an existing file, so it's more efficient
                 to write out directly.  */
              if (full_write (STDOUT_FILENO, bp, to_write) != to_write)
                die (EXIT_FAILURE, errno, "%s", _("write error"));
            }
          else if (! k)
            cwrite (new_file_flag, bp, to_write);
          n_written += to_write;
          bp += to_write;
          n_read -= to_write;
          new_file_flag = next;

          /* A line could have been so long that it skipped
             entire chunks. So create empty files in that case.  */
          while (next || chunk_end <= n_written - 1)
            {
              if (!next && bp == eob)
                {
                  /* replenish buf, before going to next chunk.  */
                  chunk_truncated = true;
                  break;
                }
              chunk_no++;
              if (k && chunk_no > k)
                return;
              if (chunk_no == n)
                chunk_end = file_size - 1; /* >= chunk_size.  */
              else
                chunk_end += chunk_size;
              if (chunk_end <= n_written - 1)
                {
                  if (! k)
                    cwrite (true, NULL, 0);
                }
              else
                next = false;
            }
        }
    }

  if (chunk_truncated)
    chunk_no++;

  /* Ensure NUMBER files are created, which truncates
     any existing files or notifies any consumers on fifos.
     FIXME: Should we do this before EXIT_FAILURE?  */
  while (!k && chunk_no++ <= n)
    cwrite (true, NULL, 0);
}

/* -n K/N: Extract Kth of N chunks.  */
static void
bytes_chunk_extract (uintmax_t k, uintmax_t n, char *buf, size_t bufsize,
                     size_t initial_read, off_t file_size)
{
  unsigned long long uni_klee_args[] = {k, n, buf, bufsize, initial_read, file_size};
  uni_klee_heap_check(uni_klee_args, 6);
  off_t start;
  off_t end;

  assert (k && n && k <= n && n <= file_size);

  start = (k - 1) * (file_size / n);
  end = (k == n) ? file_size : k * (file_size / n);

  if (initial_read != SIZE_MAX || start < initial_read)
    {
      memmove (buf, buf + start, initial_read - start);
      initial_read -= start;
    }
  else
    {
      if (lseek (STDIN_FILENO, start, SEEK_CUR) < 0)
        die (EXIT_FAILURE, errno, "%s", quotef (infile));
      initial_read = SIZE_MAX;
    }

  while (start < end)
    {
      size_t n_read;
      if (initial_read != SIZE_MAX)
        {
          n_read = initial_read;
          initial_read = SIZE_MAX;
        }
      else
        {
          n_read = safe_read (STDIN_FILENO, buf, bufsize);
          if (n_read == SAFE_READ_ERROR)
            die (EXIT_FAILURE, errno, "%s", quotef (infile));
        }
      if (n_read == 0)
        break; /* eof.  */
      n_read = MIN (n_read, end - start);
      if (full_write (STDOUT_FILENO, buf, n_read) != n_read
          && ! ignorable (errno))
        die (EXIT_FAILURE, errno, "%s", quotef ("-"));
      start += n_read;
    }
}

typedef struct of_info
{
  char *of_name;
  int ofd;
  FILE *ofile;
  int opid;
} of_t;

enum
{
  OFD_NEW = -1,
  OFD_APPEND = -2
};

/* Rotate file descriptors when we're writing to more output files than we
   have available file descriptors.
   Return whether we came under file resource pressure.
   If so, it's probably best to close each file when finished with it.  */

static bool
ofile_open (of_t *files, size_t i_check, size_t nfiles)
{
  bool file_limit = false;

  if (files[i_check].ofd <= OFD_NEW)
    {
      int fd;
      size_t i_reopen = i_check ? i_check - 1 : nfiles - 1;

      /* Another process could have opened a file in between the calls to
         close and open, so we should keep trying until open succeeds or
         we've closed all of our files.  */
      while (true)
        {
          if (files[i_check].ofd == OFD_NEW)
            fd = create (files[i_check].of_name);
          else /* OFD_APPEND  */
            {
              /* Attempt to append to previously opened file.
                 We use O_NONBLOCK to support writing to fifos,
                 where the other end has closed because of our
                 previous close.  In that case we'll immediately
                 get an error, rather than waiting indefinitely.
                 In specialised cases the consumer can keep reading
                 from the fifo, terminating on conditions in the data
                 itself, or perhaps never in the case of 'tail -f'.
                 I.e., for fifos it is valid to attempt this reopen.

                 We don't handle the filter_command case here, as create()
                 will exit if there are not enough files in that case.
                 I.e., we don't support restarting filters, as that would
                 put too much burden on users specifying --filter commands.  */
              fd = open (files[i_check].of_name,
                         O_WRONLY | O_BINARY | O_APPEND | O_NONBLOCK);
            }

          if (-1 < fd)
            break;

          if (!(errno == EMFILE || errno == ENFILE))
            die (EXIT_FAILURE, errno, "%s", quotef (files[i_check].of_name));

          file_limit = true;

          /* Search backwards for an open file to close.  */
          while (files[i_reopen].ofd < 0)
            {
              i_reopen = i_reopen ? i_reopen - 1 : nfiles - 1;
              /* No more open files to close, exit with E[NM]FILE.  */
              if (i_reopen == i_check)
                die (EXIT_FAILURE, errno, "%s",
                     quotef (files[i_check].of_name));
            }

          if (fclose (files[i_reopen].ofile) != 0)
            die (EXIT_FAILURE, errno, "%s", quotef (files[i_reopen].of_name));
          files[i_reopen].ofile = NULL;
          files[i_reopen].ofd = OFD_APPEND;
        }

      files[i_check].ofd = fd;
      if (!(files[i_check].ofile = fdopen (fd, "a")))
        die (EXIT_FAILURE, errno, "%s", quotef (files[i_check].of_name));
      files[i_check].opid = filter_pid;
      filter_pid = 0;
    }

  return file_limit;
}

/* -n r/[K/]N: Divide file into N chunks in round robin fashion.
   When K == 0, we try to keep the files open in parallel.
   If we run out of file resources, then we revert
   to opening and closing each file for each line.  */

static void
lines_rr (uintmax_t k, uintmax_t n, char *buf, size_t bufsize)
{
  bool wrapped = false;
  bool wrote = false;
  bool file_limit;
  size_t i_file;
  of_t *files IF_LINT (= NULL);
  uintmax_t line_no;

  if (k)
    line_no = 1;
  else
    {
      if (SIZE_MAX < n)
        xalloc_die ();
      files = xnmalloc (n, sizeof *files);

      /* Generate output file names. */
      for (i_file = 0; i_file < n; i_file++)
        {
          next_file_name ();
          files[i_file].of_name = xstrdup (outfile);
          files[i_file].ofd = OFD_NEW;
          files[i_file].ofile = NULL;
          files[i_file].opid = 0;
        }
      i_file = 0;
      file_limit = false;
    }

  while (true)
    {
      char *bp = buf, *eob;
      size_t n_read = safe_read (STDIN_FILENO, buf, bufsize);
      if (n_read == SAFE_READ_ERROR)
        die (EXIT_FAILURE, errno, "%s", quotef (infile));
      else if (n_read == 0)
        break; /* eof.  */
      eob = buf + n_read;

      while (bp != eob)
        {
          size_t to_write;
          bool next = false;

          /* Find end of line. */
          char *bp_out = memchr (bp, eolchar, eob - bp);
          if (bp_out)
            {
              bp_out++;
              next = true;
            }
          else
            bp_out = eob;
          to_write = bp_out - bp;

          if (k)
            {
              if (line_no == k && unbuffered)
                {
                  if (full_write (STDOUT_FILENO, bp, to_write) != to_write)
                    die (EXIT_FAILURE, errno, "%s", _("write error"));
                }
              else if (line_no == k && fwrite (bp, to_write, 1, stdout) != 1)
                {
                  clearerr (stdout); /* To silence close_stdout().  */
                  die (EXIT_FAILURE, errno, "%s", _("write error"));
                }
              if (next)
                line_no = (line_no == n) ? 1 : line_no + 1;
            }
          else
            {
              /* Secure file descriptor. */
              file_limit |= ofile_open (files, i_file, n);
              if (unbuffered)
                {
                  /* Note writing to fd, rather than flushing the FILE gives
                     an 8% performance benefit, due to reduced data copying.  */
                  if (full_write (files[i_file].ofd, bp, to_write) != to_write
                      && ! ignorable (errno))
                    {
                      die (EXIT_FAILURE, errno, "%s",
                           quotef (files[i_file].of_name));
                    }
                }
              else if (fwrite (bp, to_write, 1, files[i_file].ofile) != 1
                       && ! ignorable (errno))
                {
                  die (EXIT_FAILURE, errno, "%s",
                       quotef (files[i_file].of_name));
                }

              if (! ignorable (errno))
                wrote = true;

              if (file_limit)
                {
                  if (fclose (files[i_file].ofile) != 0)
                    {
                      die (EXIT_FAILURE, errno, "%s",
                           quotef (files[i_file].of_name));
                    }
                  files[i_file].ofile = NULL;
                  files[i_file].ofd = OFD_APPEND;
                }
              if (next && ++i_file == n)
                {
                  wrapped = true;
                  /* If no filters are accepting input, stop reading.  */
                  if (! wrote)
                    goto no_filters;
                  wrote = false;
                  i_file = 0;
                }
            }

          bp = bp_out;
        }
    }

no_filters:
  /* Ensure all files created, so that any existing files are truncated,
     and to signal any waiting fifo consumers.
     Also, close any open file descriptors.
     FIXME: Should we do this before EXIT_FAILURE?  */
  if (!k)
    {
      int ceiling = (wrapped ? n : i_file);
      for (i_file = 0; i_file < n; i_file++)
        {
          if (i_file >= ceiling && !elide_empty_files)
            file_limit |= ofile_open (files, i_file, n);
          if (files[i_file].ofd >= 0)
            closeout (files[i_file].ofile, files[i_file].ofd,
                      files[i_file].opid, files[i_file].of_name);
          files[i_file].ofd = OFD_APPEND;
        }
    }
  IF_LINT (free (files));
}

#define FAIL_ONLY_ONE_WAY()					\
  do								\
    {								\
      error (0, 0, _("cannot split in more than one way"));	\
      usage (EXIT_FAILURE);					\
    }								\
  while (0)


/* Parse K/N syntax of chunk options.  */

static void
parse_chunk (uintmax_t *k_units, uintmax_t *n_units, char *slash)
{
  *n_units = xdectoumax (slash + 1, 1, UINTMAX_MAX, "",
                         _("invalid number of chunks"), 0);
  if (slash != optarg)           /* a leading number is specified.  */
    {
      *slash = '\0';
      *k_units = xdectoumax (optarg, 1, *n_units, "",
                             _("invalid chunk number"), 0);
    }
}

#include "/root/projects/CPR/lib/argv-fuzz-inl.h"

int
main (int argc, char **argv)
{
AFL_INIT_SET02("./split", "/root/projects/CPR/patches/extractfix/coreutils/gnubug-25003/dummy");
  enum Split_type split_type = type_undef;
  size_t in_blk_size = 0;	/* optimal block size of input file device */
  size_t page_size = getpagesize ();
  uintmax_t k_units = 0;
  uintmax_t n_units = 0;

  static char const multipliers[] = "bEGKkMmPTYZ0";
  int c;
  int digits_optind = 0;
  off_t file_size = OFF_T_MAX;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  /* Parse command line options.  */

  infile = bad_cast ("-");
  outbase = bad_cast ("x");

  while (true)
    {
      /* This is the argv-index of the option we will read next.  */
      int this_optind = optind ? optind : 1;
      char *slash;

      c = getopt_long (argc, argv, "0123456789C:a:b:del:n:t:u",
                       longopts, NULL);
      if (c == -1)
        break;

      switch (c)
        {
        case 'a':
          suffix_length = xdectoumax (optarg, 0, SIZE_MAX / sizeof (size_t),
                                      "", _("invalid suffix length"), 0);
          break;

        case ADDITIONAL_SUFFIX_OPTION:
          if (last_component (optarg) != optarg)
            {
              error (0, 0,
                     _("invalid suffix %s, contains directory separator"),
                     quote (optarg));
              usage (EXIT_FAILURE);
            }
          additional_suffix = optarg;
          break;

        case 'b':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          split_type = type_bytes;
          /* Limit to OFF_T_MAX, because if input is a pipe, we could get more
             data than is possible to write to a single file, so indicate that
             immediately rather than having possibly future invocations fail. */
          n_units = xdectoumax (optarg, 1, OFF_T_MAX, multipliers,
                                _("invalid number of bytes"), 0);
          break;

        case 'l':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          split_type = type_lines;
          n_units = xdectoumax (optarg, 1, UINTMAX_MAX, "",
                                _("invalid number of lines"), 0);
          break;

        case 'C':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          split_type = type_byteslines;
          n_units = xdectoumax (optarg, 1, MIN (SIZE_MAX, OFF_T_MAX),
                                multipliers, _("invalid number of bytes"), 0);
          break;

        case 'n':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          /* skip any whitespace */
          while (isspace (to_uchar (*optarg)))
            optarg++;
          if (STRNCMP_LIT (optarg, "r/") == 0)
            {
              split_type = type_rr;
              optarg += 2;
            }
          else if (STRNCMP_LIT (optarg, "l/") == 0)
            {
              split_type = type_chunk_lines;
              optarg += 2;
            }
          else
            split_type = type_chunk_bytes;
          if ((slash = strchr (optarg, '/')))
            parse_chunk (&k_units, &n_units, slash);
          else
            n_units = xdectoumax (optarg, 1, UINTMAX_MAX, "",
                                  _("invalid number of chunks"), 0);
          break;

        case 'u':
          unbuffered = true;
          break;

        case 't':
          {
            char neweol = optarg[0];
            if (! neweol)
              die (EXIT_FAILURE, 0, _("empty record separator"));
            if (optarg[1])
              {
                if (STREQ (optarg, "\\0"))
                  neweol = '\0';
                else
                  {
                    /* Provoke with 'split -txx'.  Complain about
                       "multi-character tab" instead of "multibyte tab", so
                       that the diagnostic's wording does not need to be
                       changed once multibyte characters are supported.  */
                    die (EXIT_FAILURE, 0, _("multi-character separator %s"),
                         quote (optarg));
                  }
              }
            /* Make it explicit we don't support multiple separators.  */
            if (0 <= eolchar && neweol != eolchar)
              {
                die (EXIT_FAILURE, 0,
                     _("multiple separator characters specified"));
              }

            eolchar = neweol;
          }
          break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          if (split_type == type_undef)
            {
              split_type = type_digits;
              n_units = 0;
            }
          if (split_type != type_undef && split_type != type_digits)
            FAIL_ONLY_ONE_WAY ();
          if (digits_optind != 0 && digits_optind != this_optind)
            n_units = 0;	/* More than one number given; ignore other. */
          digits_optind = this_optind;
          if (!DECIMAL_DIGIT_ACCUMULATE (n_units, c - '0', uintmax_t))
            {
              char buffer[INT_BUFSIZE_BOUND (uintmax_t)];
              die (EXIT_FAILURE, 0,
                   _("line count option -%s%c... is too large"),
                   umaxtostr (n_units, buffer), c);
            }
          break;

        case 'd':
          suffix_alphabet = "0123456789";
          if (optarg)
            {
              if (strlen (optarg) != strspn (optarg, suffix_alphabet))
                {
                  error (0, 0,
                         _("%s: invalid start value for numerical suffix"),
                         quote (optarg));
                  usage (EXIT_FAILURE);
                }
              else
                {
                  /* Skip any leading zero.  */
                  while (*optarg == '0' && *(optarg + 1) != '\0')
                    optarg++;
                  numeric_suffix_start = optarg;
                }
            }
          break;

        case 'e':
          elide_empty_files = true;
          break;

        case FILTER_OPTION:
          filter_command = optarg;
          break;

        case IO_BLKSIZE_OPTION:
          in_blk_size = xdectoumax (optarg, 1, SIZE_MAX - page_size,
                                    multipliers, _("invalid IO block size"), 0);
          break;

        case VERBOSE_OPTION:
          verbose = true;
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }
    }

  if (k_units != 0 && filter_command)
    {
      error (0, 0, _("--filter does not process a chunk extracted to stdout"));
      usage (EXIT_FAILURE);
    }

  /* Handle default case.  */
  if (split_type == type_undef)
    {
      split_type = type_lines;
      n_units = 1000;
    }

  if (n_units == 0)
    {
      error (0, 0, "%s: %s", _("invalid number of lines"), quote ("0"));
      usage (EXIT_FAILURE);
    }

  if (eolchar < 0)
    eolchar = '\n';

  set_suffix_length (n_units, split_type);

  /* Get out the filename arguments.  */

  if (optind < argc)
    infile = argv[optind++];

  if (optind < argc)
    outbase = argv[optind++];

  if (optind < argc)
    {
      error (0, 0, _("extra operand %s"), quote (argv[optind]));
      usage (EXIT_FAILURE);
    }

  /* Check that the suffix length is large enough for the numerical
     suffix start value.  */
  if (numeric_suffix_start && strlen (numeric_suffix_start) > suffix_length)
    {
      error (0, 0, _("numerical suffix start value is too large "
                     "for the suffix length"));
      usage (EXIT_FAILURE);
    }

  /* Open the input file.  */
  if (! STREQ (infile, "-")
      && fd_reopen (STDIN_FILENO, infile, O_RDONLY, 0) < 0)
    die (EXIT_FAILURE, errno, _("cannot open %s for reading"),
         quoteaf (infile));

  /* Binary I/O is safer when byte counts are used.  */
  if (O_BINARY && ! isatty (STDIN_FILENO))
    xfreopen (NULL, "rb", stdin);

  /* Get the optimal block size of input device and make a buffer.  */

  if (fstat (STDIN_FILENO, &in_stat_buf) != 0)
    die (EXIT_FAILURE, errno, "%s", quotef (infile));

  bool specified_buf_size = !! in_blk_size;
  if (! specified_buf_size)
    in_blk_size = io_blksize (in_stat_buf);

  void *b = xmalloc (in_blk_size + 1 + page_size - 1);
  char *buf = ptr_align (b, page_size);
  size_t initial_read = SIZE_MAX;

  if (split_type == type_chunk_bytes || split_type == type_chunk_lines)
    {
      file_size = input_file_size (STDIN_FILENO, &in_stat_buf,
                                   buf, in_blk_size);
      if (file_size < 0)
        die (EXIT_FAILURE, errno, _("%s: cannot determine file size"),
             quotef (infile));
      initial_read = MIN (file_size, in_blk_size);
      /* Overflow, and sanity checking.  */
      if (OFF_T_MAX < n_units)
        {
          char buffer[INT_BUFSIZE_BOUND (uintmax_t)];
          die (EXIT_FAILURE, EOVERFLOW, "%s: %s",
               _("invalid number of chunks"),
               quote (umaxtostr (n_units, buffer)));
        }
      /* increase file_size to n_units here, so that we still process
         any input data, and create empty files for the rest.  */
      file_size = MAX (file_size, n_units);
    }

  /* When filtering, closure of one pipe must not terminate the process,
     as there may still be other streams expecting input from us.  */
  if (filter_command)
    {
      struct sigaction act;
      sigemptyset (&newblocked);
      sigaction (SIGPIPE, NULL, &act);
      if (act.sa_handler != SIG_IGN)
        sigaddset (&newblocked, SIGPIPE);
      sigprocmask (SIG_BLOCK, &newblocked, &oldblocked);
    }

  switch (split_type)
    {
    case type_digits:
    case type_lines:
      lines_split (n_units, buf, in_blk_size);
      break;

    case type_bytes:
      bytes_split (n_units, buf, in_blk_size, SIZE_MAX, 0);
      break;

    case type_byteslines:
      line_bytes_split (n_units, buf, in_blk_size);
      break;

    case type_chunk_bytes:
      if (k_units == 0)
        bytes_split (file_size / n_units, buf, in_blk_size, initial_read,
                     n_units);
      else
        bytes_chunk_extract (k_units, n_units, buf, in_blk_size, initial_read,
                             file_size);
      break;

    case type_chunk_lines:
      lines_chunk_split (k_units, n_units, buf, in_blk_size, initial_read,
                         file_size);
      break;

    case type_rr:
      /* Note, this is like 'sed -n ${k}~${n}p' when k > 0,
         but the functionality is provided for symmetry.  */
      lines_rr (k_units, n_units, buf, in_blk_size);
      break;

    default:
      abort ();
    }

  IF_LINT (free (b));

  if (close (STDIN_FILENO) != 0)
    die (EXIT_FAILURE, errno, "%s", quotef (infile));
  closeout (NULL, output_desc, filter_pid, outfile);

  return EXIT_SUCCESS;
}
