#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H


#include "defines.h"
#include "bistack.h"
#include "nvmem.h"
#include "eeprom.h"
#include "cell.h"


typedef void *(*pointer_resolver)(BISTACK *, void *);


void *codemem_resolver(BISTACK *bs, void *thing) {
  return NULL;
}


void *nvmem_resolver(BISTACK *bs, void *thing) {
  return NULL;
}


void *mem_resolver(BISTACK *bs, void *thing) {
  return NULL;
}


pointer_resolver get_resolver(void *ptr) {
  const uintptr_t ptr_int = (uintptr_t)ptr;
  const char top_two_bits = ptr_int >> ((sizeof(ptr_int) << 3) - 2);
  if (top_two_bits >= 2) {
    // first bit in ptr is a 1, so its in CODE mem
    return codemem_resolver;
  } else if (top_two_bits == 1) {
    return nvmem_resolver;
  } else {
    return mem_resolver;
  }
}


typedef struct symbol_binding {
    CELL symbol_cell;
    CELL bound_cell;
    struct symbol_binding *next;
} SYMBOL_BINDING;


typedef struct symbol_binding_frame {
    SYMBOL_BINDING *symbol_binding;
    struct symbol_binding_frame *next;
} SYMBOL_BINDING_FRAME;


typedef struct environment {
    BISTACK *bs;
    SYMBOL_BINDING_FRAME *frame;
} ENVIRONMENT;


CELL *env_find_symbol(CELL *symbol) {
  return NULL;
}

eeprom_addr_t eeprom_cell_ptr_to_addr(CELL *cell) {
    return (eeprom_addr_t)(cell);
}   

CELL* eeprom_addr_to_cell_ptr(eeprom_addr_t addr) {
    return (CELL *)addr;
}

#endif
