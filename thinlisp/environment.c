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
    while (addr < eeprom_get_end()) {
        eeprom_read(addr, &itr, sizeof(EEPROM_BLOCK));
    
        if (!itr.is_free && itr.type == EEPROM_BLOCK_TYPE_LISP) {
            // load symbol cell
            CELL *symbol_cell = bistack_heapalloc(env->bs, sizeof(CELL));
            cell_load(symbol_cell, eeprom_addr_to_cell_ptr(addr));

            dassert(cell_is_symbol(symbol_cell), EEPROM_BINDING_NOT_SYMBOL);
            dassert(
                cell_symbol_is_ptr(symbol_cell), EEPROM_BINDING_NOT_SYMBOL_PTR);
            
            addr = addr + sizeof(EEPROM_BLOCK) + sizeof(CELLHEADER);
            symbol_cell->string_ptr = eeprom_addr_to_char_ptr(addr);

            addr = addr + cell_symbol_length(symbol_cell);

            CELL *bound_cell = bistack_heapalloc(env->bs, sizeof(CELL));
            cell_load(bound_cell, eeprom_addr_to_cell_ptr(addr));

            addr = eeprom_cell_ptr_to_addr(
                    cell_advance(env->bs, eeprom_addr_to_cell_ptr(addr)));
        }
    }
}
