#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdarg.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>

#include "defines.h"

enum {
  BISTACK_OUT_OF_MEMORY=1,
  
  BISTACK_REWIND_TOO_FAR,
  BISTACK_DROPMARK_TOO_FAR,
  BISTACK_LOCK_HELD,

  CELL_LIST_PTR_IN_NVMEM,
  CELL_SYMBOL_PTR_IN_NVMEM,
  
  COMPILED_SIZE_UNEXPECTED,

  EEPROM_BINDING_NOT_SYMBOL_PTR,
  EEPROM_BINDING_NOT_SYMBOL,
  
  NVMEM_READ_ERROR,
  NVMEM_WRITE_ERROR,
  NVMEM_OUT_OF_MEMORY,
  NVMEM_ADDRESS_ERROR,
  
  READER_NON_NUMERIC_IN_INTEGER,
  READER_SYNTAX_ERROR,
  READER_STATE_ERROR,
  READER_SYNTAX_SPURIOUS_LIST_TERMINATOR,
  READER_WHITESPACE_IN_PREFIX,
  READER_COMMA_ILLEGAL_OUTSIDE_OF_BACKQUOTE,
  READER_LOCK_REUSED,
  
  RUNTIME_SYMBOL_IS_UNBOUND,
  RUNTIME_TWO_SYMBOL_ARGUMENTS_EXPECTED,
  RUNTIME_INTEGER_EXPECTED,
  RUNTIME_EQUAL_INSUFFICIENT_ARGUMENTS,
  RUNTIME_SYMBOL_EXPECTED,
  RUNTIME_LIST_EXPECTED,
  RUNTIME_TWO_ARGUMENTS_EXPECTED,
    
  UNEXPECTED_BRANCH,

  VLIST_INDEX_ERROR,
  VLIST_SHIFT_ON_EMPTY,
};


char *thrown_error_to_string(char err);

extern jmp_buf __jmpbuff;

void lerror(uint16_t exctype, char *err, ...);
void lassert(uint16_t truefalse, uint16_t exctype, ...);
void dassert(uint16_t truefalse, uint16_t exctype, ...);

extern void *mem_offset;

inline address_t mem_to_addr(void *ptr) {
    // return offset in 16meg(24-bit) window
    address_t addr = ((address_t)ptr) & ((1<<ADDRESS_BITS) - 1);    
    if (mem_offset == 0) {
        mem_offset = ptr - addr;
    }
    return addr;
}

inline char *addr_to_chrptr(address_t addr) {
    return mem_offset + addr;
}

inline char *addr_to_mem(address_t addr) {
    return mem_offset + addr;
}

inline char *addr_to_voidptr(address_t addr) {
    return mem_offset + addr;
}

#endif
