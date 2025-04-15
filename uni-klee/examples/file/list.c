#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "klee/klee.h"
#define BUFF_SIZE 4
int myglob = 'a';

struct myarg {
  char is_head;
  short a;
  struct myarg *next;
  int b;
  char buff[BUFF_SIZE];
};

int get_sign(struct myarg *x, FILE *fp) {
  int result = 0;
  int tmp;
  char str[16];
  fgets(str, 16, fp);
  printf("str: %s\n", str);
  x->a = tmp;
  assert(x->is_head);
  if (x->a < 0)
    return result;
  struct myarg *y = x->next;
  while (y) {
    y = y->next;
    result++;
    // klee_print_expr("result", result);
    // klee_print_expr("y", y);
  }
  assert(result == x->a);
  if (result > BUFF_SIZE)
    return result;
  else
    return result + 2 * x->buff[result];
}

int main(int argc, char * argv[]) {
  int a;
  a = atoi(argv[1]);
  struct myarg *head = malloc(sizeof(struct myarg));
  head->is_head = 1;
  head->a = a;
  head->b = 0;
  head->next = NULL;
  for (int j = 0; j < BUFF_SIZE; j++) {
    head->buff[j] = myglob;
  }
  struct myarg *prev = head;
  for (int i = 0; i < a; i++) {
    struct myarg *myx = malloc(sizeof(struct myarg));
    prev->next = myx;
    myx->next = NULL;
    myx->is_head = 0;
    myx->a = i;
    myx->b = i + 1;
    prev = myx;
    for (int j = 0; j < BUFF_SIZE; j++) {
      myx->buff[j] = myglob + j;
    }
  }
  FILE *fp = fopen("list.txt", "r");
  int result = get_sign(head, fp);
  printf("%d\n", result);
  while (head != NULL) {
    struct myarg *next = head->next;
    free(head);
    head = next;
  }
  return 0;
}