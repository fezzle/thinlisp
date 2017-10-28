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
    eeprom_addr_t addr = eeprom_get_start();

    SYMBOL_BINDING_FRAME *frame;

    BISTACK *bs = env->bs;
    bistack_lock_t bistack_lock = bistack_lockstack(bs);

    // assoc cells are a list of 2-tuples containing symbol:cell bindings
    CELL *assoc_cell = bistack_pushstack(bs, bistack_lock, sizeof(CELL));
    cell_list_init(assoc_cell, AST_NONE, FALSE, 0);
        
    while (addr < eeprom_get_end()) {
        eeprom_read(addr, &itr, sizeof(EEPROM_BLOCK));
        addr += sizeof(EEPROM_BLOCK);
    
        if (itr.type == EEPROM_BLOCK_TYPE_LISP) {
            // create tuple association cell
            // [0] 2-tuple (list) cell
            // [1] symbol cell
            // [2] bound cell
            CELL *cells = bistack_pushstack(bs, bistack_lock, 3 * sizeof(CELL));
            cell_list_init(&cells[0], AST_NONE, FALSE, 2);

            // load symbol cell
            cell_load(&cells[1], eeprom_addr_to_cell_ptr(addr));

            dassert(cell_is_symbol(&cells[1]), EEPROM_BINDING_NOT_SYMBOL);
            dassert(
                cell_symbol_is_ptr(&cells[1]), EEPROM_BINDING_NOT_SYMBOL_PTR);
            
            addr = addr + sizeof(EEPROM_BLOCK) + sizeof(CELLHEADER);
            cells[1].string_ptr = eeprom_addr_to_char_ptr(addr);

            addr = addr + cell_symbol_length(&cells[1]);

            cell_load(&cells[2], eeprom_addr_to_cell_ptr(addr));

            if (CELL_IS_LIST(cells[2])) {               
                addr = eeprom_cell_ptr_to_addr(
                    cell_advance(env->bs, eeprom_addr_to_cell_ptr(addr)));

            } else if (CELL_IS_SYMBOL(cells[2])) {
                cells[2].header.Symbol.is_ptr = TRUE;

                cells[2].string_ptr = eeprom_addr_to_char_ptr(
                    addr + sizeof(EEPROM_BLOCK) + sizeof(CELLHEADER));    
            }
        }
    }
}
