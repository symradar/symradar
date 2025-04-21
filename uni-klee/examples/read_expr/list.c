#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <klee/klee.h>
int main(int argc, char *argv[]) {
  long long i;
  klee_make_symbolic(&i, sizeof(i), "i");
  long long x;
  klee_make_symbolic(&x, sizeof(x), "x");
  int y = (int)x;
  int is = (int)i;
  // klee_assume(x == (long long)y);
  // klee_assume(i == (long long)is);
  klee_assume(0L <= i);
  klee_assume(i <= 42000000L);
  klee_assume(0L <= x);
  klee_assume(x <= 42000000L);
  if (2 * i > x) {
    return 1;
  }
  if (2 * is > y) {
    return 2;
  }
  if (2 * is > x) {
    return 3;
  }
  if (2 * i > y) {
    return 4;
  }
  return 0;
}