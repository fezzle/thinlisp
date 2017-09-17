#include "defines.h"
#include "cell.h"
#include "runtime.h"


/**
 * @return 1 if a > b, -1 if a < b, 0 if equal
 */
 int8_t cell_symbol_compare(CELL *a, CELL *b) {
    dassert(
        a->header.Symbol.type == AST_SYMBOL && 
            b->header.Symbol.type == AST_SYMBOL,
        RUNTIME_TWO_ARGUMENTS_EXPECTED);
    
    char *a_chars = a->header.Symbol.is_ptr ? a->string_ptr : a->string;
    char *b_chars = b->header.Symbol.is_ptr ? b->string_ptr : b->string;

    symbol_size_t a_len = a->header.Symbol.length;
    symbol_size_t b_len = b->header.Symbol.length;

    while (a_len && b_len) {
        char a_char = DEREF(a_chars);
        char b_char = DEREF(b_chars);
        if (a_char < b_char) {
            return 1;
        } else if (a_char > b_char) {
            return -1;
        }
        a_chars++;
        a_len--;
        b_chars++;
        b_len--;
    }
    if (a_len > b_len) {
        return -1;
    } else if (a_len < b_len) {
        return 1;
    } else {
        return 0;
    }
}