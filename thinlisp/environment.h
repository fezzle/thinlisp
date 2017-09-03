#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "defines.h"
#include "bistack.h"
#include "nvmem.h"

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
  const uintptr_t ptrint = (uintptr_t)ptr;
  const char top_two_bits = ptrint >> ((sizeof(ptrint) << 3) - 2);
  if (top_two_bits >= 2) {
    // first bit in ptr is a 1, so its in CODE mem
    return codemem_resolver;
  } else if (top_two_bits == 1) {
    return nvmem_resolver;
  } else {
    return mem_resolver;
  }
}

typedef struct symboltable_entry {
  SYMBOL *symbol;
  CELL *val;
} SYMBOLTABLE_ENTRY;

typedef struct symboltable {
  SYMBOLTABLE_ENTRY *entries;
} SYMBOLTABLE;

typedef struct environment {
  BISTACK *bs;
  char dir;
  SYMBOLTABLE *builtins;
  SYMBOLTABLE *globals;
  SYMBOLTABLE *globals2;
  SYMBOLTABLE *locals;
} ENVIRONMENT;

CELLHEADER *env_find_symbol(SYMBOL *symbol) {
  return NULL;
}


#endif
