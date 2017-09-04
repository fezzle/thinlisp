
#ifndef EVAL_H
#define EVAL_H

#include "bistack.h"
#include "defines.h"
#include "runtime.h"

inline CELLHEADER *make_integer(BISTACK *bs, int16_t value) {
    CELLHEADER *cell = (CELLHEADER *)bistack_alloc(bs, sizeof(CELLHEADER));
    cell->Integer.type = AST_INTEGER;
    cell->Integer.sign = value >= 0;
    cell->Integer.value = value >= 0 ? value : -value;
    return cell;
}


inline CELLHEADER *make_symbol(BISTACK *bs, char *str, string_size_t len) {
    CELLHEADER *cell = (CELLHEADER *)bistack_alloc(
        bs, sizeof(CELLHEADER) + len);
    cell->Symbol.type = AST_SYMBOL;
    cell->Symbol.prefix = AST_NONE;
    cell->Symbol.length = len;
    char *dest = (char*)(cell + 1);
    while (len--) {
        *dest++ = *str++;
    }
    return cell;
}

CELLHEADER *equal(BISTACK *bs, CELLHEADER *first, CELLHEADER *second);

#endif