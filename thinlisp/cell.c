#include "defines.h"
#include "cell.h"
#include "bistack.h"
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


CELL *cell_advance(BISTACK *bs, CELL *cellptr) {
    /**
     * Advances to next cell.
     */
    CELLHEADER cellheader_buf;
    cellheader_load((CELLHEADER*)cellptr, &cellheader_buf);

    dassert(CELLHEADER_IS_LIST(cellheader_buf), RUNTIME_LIST_EXPECTED);
    
    bistack_heapmark(bs);
    struct frame {
        list_size_t counter;
        struct frame *next;
    } *current_frame = NULL;

    while (TRUE) {
        while (current_frame && current_frame->counter == 0) {
            current_frame = current_frame->next;
            if (current_frame == NULL) {
                break;
            }
        }
        cellheader_load((CELLHEADER*)cellptr, &cellheader_buf);
        if (CELLHEADER_IS_INTEGER(cellheader_buf)) {
            cellptr = (CELL*)((byte_ptr_t)cellptr + sizeof(CELL));
            current_frame->counter--;
        } else if (CELLHEADER_IS_SYMBOL(cellheader_buf)) {
            if (CELLHEADER_SYMBOL_IS_PTR(cellheader_buf)) {
                cellptr = (CELL*)(
                    (byte_ptr_t)cellptr + sizeof(CELL));
            } else {
                cellptr = (CELL*)(
                    (byte_ptr_t)cellptr + 
                    CELLHEADER_SYMBOL_LENGTH(cellheader_buf));
            }
            current_frame->counter--;
        } else if (CELLHEADER_IS_LIST(cellheader_buf)) {
            if (CELLHEADER_LIST_IS_PTR(cellheader_buf)) {
                cellptr = (CELL*)((byte_ptr_t)cellptr + sizeof(CELL));
            } else {
                struct frame *new_frame = (struct frame*)(
                    bistack_heapalloc(bs, sizeof(struct frame)));
                new_frame->counter = CELLHEADER_LIST_LENGTH(cellheader_buf);
                new_frame->next = current_frame;
                cellptr = (CELL*)((byte_ptr_t)cellptr + sizeof(CELLHEADER));
                current_frame->counter--;
                current_frame = new_frame;
            }
        } else {
            dassert(FALSE, UNEXPECTED_BRANCH);
        }
    }

    bistack_heaprewind(bs);
    return cellptr;
}

/**
 * TODO: update if CELLHEADER size changes
 */
CELL *cell_load(CELL *src, CELL *dest) {
    cellheader_load(&src->header, &dest->header);
    if (cell_is_list(src)) {
        dest->header.List.is_ptr = TRUE;
        if (src->header.List.is_ptr) {
            ((CELL*)dest)->cells_ptr = ((CELL*)src)->cells_ptr;
        } else {
            ((CELL*)dest)->cells_ptr = ((CELL*)src)->cells;
        }
    } else if (cell_is_symbol(src)) {
        dest->header.Symbol.is_ptr = TRUE;
        if (src->header.Symbol.is_ptr) {
            ((CELL*)dest)->string_ptr = ((CELL*)src)->string_ptr;
        } else {
            ((CELL*)dest)->string_ptr = ((CELL*)src)->string;
        }
    }
    return (CELL*)dest;
}
