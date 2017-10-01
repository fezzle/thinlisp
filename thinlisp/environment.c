#include "bistack.h"
#include "utils.h"
#include "cell.h"
#include "eeprom.h"

#include "environment.h"



CELL *frame_find_symbol(SYMBOL_BINDING_FRAME *frame, CELL *symbol_cell) {
    SYMBOL_BINDING_FRAME *frame_itr = frame;
    while (frame_itr) {
        SYMBOL_BINDING *binding_itr = frame_itr->symbol_binding;
        while (binding_itr) {
            if (cell_symbol_compare(binding_itr->symbol_cell, symbol_cell)) {
                return binding_itr->bound_cell;
            } 
            binding_itr = binding_itr->next;
        }
        frame_itr = frame_itr->next;
    }
}


void environment_boostrap_from_eeprom(ENVIRONMENT *env) {
    EEPROM_BLOCK itr;
    eeprom_addr_t addr = 0;
    while (addr < eeprom_get_size()) {
        eeprom_read(addr, &itr, sizeof(EEPROM_BLOCK));
    
        if (!itr.is_free && itr.type == EEPROM_BLOCK_TYPE_LISP) {
            // load symbol cell
            CELL *symbol_cell = bistack_heapalloc(env->bs, sizeof(CELL));
            eeprom_read(addr + sizeof(EEPROM_BLOCK), symbol_cell, sizeof(CELL));

            dassert(CELL_IS_SYMBOL(*symbol_cell), EEPROM_BINDING_NOT_SYMBOL);
            symbol_cell->header.Symbol.is_ptr = TRUE;
            symbol_cell->string_ptr = EEPROM_ADDR_TO_CHAR_PTR(
                addr + sizeof(EEPROM_BLOCK) + sizeof(CELLHEADER));
            
            
            addr += (
                addr + sizeof(EEPROM_BLOCK) + sizeof(CELLHEADER) + 
                CELL_SYMBOL_LENGTH(*symbol_cell));

            CELL *bound_cell = bistack_heapalloc(env->bs, sizeof(CELL));
            eeprom_read(addr + sizeof(EEPROM_BLOCK), bound_cell, sizeof(CELL));

            if (CELL_IS_LIST(*bound_cell)) {
                bound_cell->header.List.is_ptr = TRUE;
                bound_cell->cells_ptr = (CELL*)EEPROM_ADDR_TO_PTR(
                    addr + sizeof(EEPROM_BLOCK) + sizeof(CELLHEADER));
            
            } else if (CELL_IS_SYMBOL(*bound_cell)) {
                bound_cell->header.Symbol.is_ptr = TRUE;
                symbol_cell->string_ptr = (char*)EEPROM_ADDR_TO_PTR(
                    addr + sizeof(EEPROM_BLOCK) + sizeof(CELLHEADER));    
            }
        }
    }
}


/**
 * Marks a gc point in the bistack and pushes a symbol table in the environment
 */ 
SYMBOLTABLE *env_pushsymtable(ENVIRONMENT *env) {
  bistack_pushdir(env->bs, env->dir);
  bistack_mark(env->bs);
  SYMBOLTABLE *st = bistack_alloc(env->bs, sizeof(SYMBOLTABLE));
  st->avlnode = NULL;
  st->outer = env->innerbindings;
  env->innerbindings = st;
  if (!env->globalbindings) {
    env->globalbindings = st;
  }
  bistack_popdir(env->bs);
  return st;
}


/**
 * Removes the last symbol table from the environment and frees the last mark.
 */
void env_popsymtable(ENVIRONMENT *env) {
  assert(env->innerbindings != NULL);
  
  bistack_pushdir(env->bs, env->dir);
  bistack_rewind(env->bs);
  if (env->innerbindings) {
    env->innerbindings = env->innerbindings->outer;
  }
  bistack_popdir(env->bs);
}

/**
 * Binds a symbol in the environment.
 */
void env_bind_symbol(ENVIRONMENT *env, SYMBOL sym, VALUE v) {
  assert(env->innerbindings != NULL);
  
  bistack_pushdir(env->bs, env->dir);
  env->innerbindings->avlnode = avl_insert(env->innerbindings->avlnode,
					   env->bs, sym, v);
  bistack_popdir(env->bs);
}
