#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdarg.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>

#include "defines.h"

enum {
  BISTACK_OUT_OF_MEMORY=1,
  READER_SYNTAX_ERROR,
  READER_STATE_ERROR,
  READER_SYNTAX_SPURIOUS_LIST_TERMINATOR,
  BISTACK_REWIND_TOO_FAR,
  BISTACK_DROPMARK_TOO_FAR,
  VLIST_INDEX_ERROR,
  VLIST_SHIFT_ON_EMPTY,
  NVMEM_READ_ERROR,
  NVMEM_WRITE_ERROR,
  NVMEM_OUT_OF_MEMORY,
  NVMEM_ADDRESS_ERROR,
  RUNTIME_SYMBOL_IS_UNBOUND,
  RUNTIME_INTEGER_EXPECTED,
  RUNTIME_EQUAL_INSUFFICIENT_ARGUMENTS,
  RUNTIME_SYMBOL_EXPECTED,
  RUNTIME_LIST_EXPECTED,
  RUNTIME_TWO_ARGUMENTS_EXPECTED,
  EEPROM_BINDING_NOT_SYMBOL,
};


char *thrown_error_to_string(char err);

extern jmp_buf __jmpbuff;

extern CELLHEADER *CELL_FALSE;
extern CELLHEADER *CELL_TRUE;

void lerror(uint16_t exctype, char *err, ...);
void lassert(uint16_t truefalse, uint16_t exctype, ...);
void dassert(uint16_t truefalse, uint16_t exctype, ...);

#endif
