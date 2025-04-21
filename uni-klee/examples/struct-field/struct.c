#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFF_SIZE 4
int myglob = 'a';

struct myarg {
  short a;
  int b;
  char buff[BUFF_SIZE];
};

int get_sign(struct myarg *x) {
  if (x->a < 0)
    return 0;
  for (int i = 0; i < BUFF_SIZE; i++) {
    x->buff[i] = myglob + i + x->b;
  }

  if (x->a > BUFF_SIZE)
    return -1;
  else
    return x->buff[x->a];
}

int main(int argc, char * argv[]) {
  // int a;
  // a = atoi(argv[1]);
  struct myarg myx;
  void *start = &myx.a;
  // klee_make_symbolic(&myx.a, sizeof(myx.a), "myx.a");
  void *symbolic_data = malloc(sizeof(myx.a));
  klee_make_symbolic(symbolic_data, sizeof(myx.a), "myx.a");
  memcpy(start, symbolic_data, sizeof(myx.a));
  free(symbolic_data);
  myx.b = 1;
  int result = get_sign(&myx);
  printf("%d\n", result);
  return 0;
}