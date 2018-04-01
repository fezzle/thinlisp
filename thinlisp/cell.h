#ifndef CELL_H
#define CELL_H

#include <stdint.h>

#include "defines.h"
#include "bistack.h"
#include "runtime.h"

// low 2 bits of AST type are numeration
// high 6 bits are bitfield
#define AST_NONE  0
#define AST_SYMBOL 1
#define AST_INTEGER 2
#define AST_LIST 3
#define AST_STRING 4


typedef struct cell {
    struct {
        uint32_t type : 2;
        uint32_t length : 6; // should be 6
        uint32_t address : 24; 
    } symbol;

    struct {
        uint32_t type : 2;
        int32_t value : (32 - 2);  // signed 30 bit must be sign extended to 32
    } integer;

    struct {
        uint32_t type: 2;
        uint32_t length : 6;
        uint32_t address : 24; 
    } list;
} CELL;


inline int32_t cell_integer_val(CELL *cell) {
    return cell->integer.value;
}

inline bool_t cell_is_list(CELL *cell) {
    return cell->list.type == AST_LIST;
}

inline bool_t cell_is_symbol(CELL *cell) {
    return cell->symbol.type == AST_SYMBOL;
}

inline string_size_t cell_symbol_length(CELL *cell) {
    return cell->symbol.length;
}

inline bool_t cell_is_integer(CELL *cell) {
    return cell->integer.type == AST_INTEGER;
}


CELL *cell_init_list(CELL *cell, list_size_t length) {
    cell->list.type = AST_LIST;
    cell->list.length = length;
    return cell;
}

CELL *cell_init_symbol(CELL *cell, string_size_t length) {
    cell->symbol.type = AST_SYMBOL;
    cell->symbol.length = length;
    cell->symbol.address = 0;
}

bool_t cell_symbol_compare(CELL *a, CELL *b);

CELL *cell_init_integer(CELL *cell, int32_t val) {
    cell->integer.type = AST_INTEGER;
    cell->integer.value = val;
}

typedef char (*get_char_n_ptr)(void *, string_size_t);
typedef struct string_function {
    get_char_n_ptr get_char;
    CELL *arg;
} STRING_DEREF;

char get_pgm_string(void *arg, string_size_t n) {
    return PGM_READ_BYTE((char*)arg + n);
}


#endif