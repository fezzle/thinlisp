#include <stdio.h>
#include <setjmp.h>

#include "runtime.h"

void lerror(uint16_t exctype, char *err, ...) {
  #ifndef LIMITED_ENVIRONMENT
  printf("ERROR: %s\n", err);
  #endif
  longjmp(__jmpbuff, exctype);
}

void lassert(uint16_t truefalse, uint16_t exctype, ...) {
  if (!truefalse) {
    lerror(exctype, PSTR("ASSERTION FAILED"));
  }
}
