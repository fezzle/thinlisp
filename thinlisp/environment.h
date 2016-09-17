#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "bistack.h"
#include "main.h"
#include "nvmem.h"

typedef struct symboltable_entry {
  symbol_t symbol;
  char *str;
} SYMBOLTABLE_ENTRY;


typedef struct symboltable {
  uint8_t is_in_memory:1;
  uint8_t entrycount:7;
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


/*
** Returns the symbol as char* corresponding to symbolcandidate.
** If str is supplied and does not match the corresponding char*, return NULL.
*/
char *symboltable_findsymbol(SYMBOLTABLE *table,
				char *str,
				uint8_t strlength,
				uint8_t symbolcandidate) {
  // binary search
  uint8_t lastpos = -1;
  uint8_t position = table->entrycount >> 1;
  SYMBOLTABLE_ENTRY entrybuff;
  while (position >= 0) {
    SYMBOLTABLE_ENTRY *entry;
    if (table->is_in_memory) {
      entry = &table->entries[position];
    } else {
      nvmem_fetch(&entrybuff, &table->entries[position], sizeof(entrybuff));
      entry = &entrybuff;
    }
    if (entry->
  }
    
  
  if (table->is_in_memory) {
    
    
    
    for (int i=0; i<table->entrycount; i++) {
      if (entry->symbol == symbolcandidate) {
	if (str && strncmp(entry->str, str,
		     strlength < entry->length ? strlength : entry->length) == 0
	  ) {
	  return entry->str;
	}
      }
    }
  }
  return NULL;
}

symbol_t environment_findsymbol(char *str, uint8_t strlen, symbol_t symbolcandidate) {
  // check builtins
  
}


/**
 * Marks a gc point in the bistack and pushes a symbol table in the environment
 */ 
SYMBOLTABLE *env_pushsymtable(ENVIRONMENT *env);

/**
 * Removes the last symbol table from the environment and frees the last mark.
 */
void env_popsymtable(ENVIRONMENT *env);

/**
 * Binds a symbol in the environment.
 */
void env_bind_symbol(ENVIRONMENT *env, SYMBOL sym, VALUE v);
}

#endif
