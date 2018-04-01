#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdint.h>
#include <stdio.h>


typedef struct {
  int64_t a : 10;
  int64_t b : 54;
  char (*fn)();
} T;


int main() {
  T t;
  t.a = 1023;
  t.b = -100;
  t.fn = NULL;

  uint64_t ub = t.b;
  int64_t sb = t.b;

  printf("sizeof(T):%lu, a:%lld  unsigned: %llu    signed: %lld\n", sizeof(T), t.a, ub, sb);
  printf("fn:%llu\n", t.fn);
}
