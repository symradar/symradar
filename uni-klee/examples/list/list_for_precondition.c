#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define BUFF_SIZE 4
int myglob = 'a';
int uni_klee_patch_id;

void klee_select_patch(int *patch_id) { *patch_id = 0; }

void uni_klee_add_patch(int *patch_results, int patch_id, int result) {
  patch_results[patch_id] = result;
}

int uni_klee_choice(int *patch_results, int patch_id) {
  return patch_results[patch_id];
}

// UNI_KLEE_START
int __cpr_choice(char *lid, char *typestr, long long *rvals, char **rvals_ids,
                 int rvals_size, int **lvals, char **lvals_ids,
                 int lvals_size) {
  // int patch_results[4096];
  int result;
  long long x = rvals[0];
  long long y = rvals[1];
  long long constant_a;
  int patch_results[106];
  // Patch buggy # 0
  result = (0);
  uni_klee_add_patch(patch_results, 0, result);
  // Patch 1-0 # 1
  result = (x == y);
  uni_klee_add_patch(patch_results, 1, result);
  // Patch 2-0 # 2
  result = (x >= y);
  uni_klee_add_patch(patch_results, 2, result);
  // Patch 3-0 # 3
  result = (x == 4);
  uni_klee_add_patch(patch_results, 3, result);
  klee_select_patch(&uni_klee_patch_id);
  return uni_klee_choice(patch_results, uni_klee_patch_id);
}

void klee_assume(unsigned long long x) {  }
int __uni_klee_true() { return 1; }

struct myarg {
  char is_head;
  short a;
  struct myarg *next;
  int b;
  char buff[BUFF_SIZE];
};

int get_sign(struct myarg *x, struct myarg *y) {
  if (__uni_klee_true()) {
    int result = 1;
    struct myarg *nodex = x->next;
    while (nodex) {
      nodex = nodex->next;
      result++;
    }
    klee_assume(x->a == 4);
    // klee_assume(result == 4);
    return result;
  }
  int result = 1;
  assert(x->is_head);
  if (x->a < 0)
    return result;
  struct myarg *nodex = x->next;
  struct myarg *nodey = y->next;
  while (nodex) {
    nodex = nodex->next;
    result++;
  }
  int result2 = 0;
  while (nodey) {
    nodey = nodey->next;
    result2++;
  }
  if (__cpr_choice("L9", "bool",
                   (long long[]){(long long)result, (long long)x->a},
                   (char *[]){"x", "y"}, 2, (int *[]){}, (char *[]){}, 0))
    return 0;
  assert(result == 4);
  // assert(result2 == y->a);
  if (result > BUFF_SIZE)
    return result;
  else
    return result + 2 * x->buff[result];
}

int main(int argc, char * argv[]) {
  int a = 4;
  // atoi(argv[1]);
  struct myarg *head = malloc(sizeof(struct myarg));
  struct myarg *y = malloc(sizeof(struct myarg));
  head->is_head = 1;
  head->a = a;
  head->b = 0;
  head->next = NULL;
  for (int j = 0; j < BUFF_SIZE; j++) {
    head->buff[j] = myglob;
  }
  y->is_head = 1;
  y->a = a;
  y->b = 0;
  y->next = NULL;
  struct myarg *prev = head;
  struct myarg *prevy = y;
  for (int i = 0; i < a; i++) {
    struct myarg *myx = malloc(sizeof(struct myarg));
    struct myarg *myy = malloc(sizeof(struct myarg));
    prev->next = myx;
    myx->next = NULL;
    myx->is_head = 0;
    myx->a = i;
    myx->b = i + 1;
    prev = myx;
    for (int j = 0; j < BUFF_SIZE; j++) {
      myx->buff[j] = myglob + j;
    }
    prevy->next = myy;
    myy->next = NULL;
    myy->is_head = 0;
    myy->a = i;
    myy->b = i + 1;
    prevy = myy;
    for (int j = 0; j < BUFF_SIZE; j++) {
      myy->buff[j] = myglob + j;
    }
  }
  int result = get_sign(head, y);
  printf("%d\n", result);
  while (head != NULL) {
    struct myarg *next = head->next;
    free(head);
    head = next;
  }
  return 0;
}