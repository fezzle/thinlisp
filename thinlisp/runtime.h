#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdarg.h>
#include <setjmp.h>
#include <stdio.h>

#include "defines.h"

enum {
  BISTACK_OUT_OF_MEMORY=1,
  READER_SYNTAX_ERROR,
  READER_INTEGER_ERROR,
  READER_STATE_ERROR,
  VLIST_INDEX_ERROR,
  VLIST_SHIFT_ON_EMPTY,
  NVMEM_READ_ERROR,
  NVMEM_WRITE_ERROR,
  NVMEM_OUT_OF_MEMORY,
};

extern jmp_buf __jmpbuff;

void lerror(uint16_t exctype, char *err, ...);
void lassert(uint16_t truefalse, uint16_t exctype, ...);

#ifndef POSIX
#define POSIX
#endif


#endif
