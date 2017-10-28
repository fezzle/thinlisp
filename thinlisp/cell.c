#include <string.h>

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

typedef void *(*cell_visitor)(BISTACK *bs, CELL *cell, void *context);

void *cell_visit_pass(BISTACK *bs, CELL *cell, void *context) {
    return context;
}

void *nvmem_cell_enter_list(BISTACK *bs, CELL *cell, void *context) {

}

void *nvmem_cell_leave_list(BISTACK *bs, CELL *cell, void *context) {
    
}

void *nvmem_cell_visit_symbol(BISTACK *bs, CELL *cell, void *context) {
    
}

void *nvmem_cell_visit_integer(BISTACK *bs, CELL *cell, void *context) {
    
}


CELL *nvmem_cell_iterate(
        BISTACK *bs, 
        CELL *cellptr,
        cell_visitor enter_list,
        cell_visitor leave_list,
        cell_visitor visit_symbol,
        cell_visitor visit_integer,
        void *context) {
    /**
     * Advances to next cell.
     */
    const void *mark = bistack_heapmark(bs);

    CELL cell_buf;

    struct frame {
        CELL cell_buf;
        list_size_t counter;
        struct frame *next;
    } *current_frame = NULL;   

    while (TRUE) {
        while (current_frame && current_frame->counter == 0) {
            context = leave_list(bs, &current_frame->cell_buf, context);
            current_frame = current_frame->next;
            if (current_frame == NULL) {
                break;
            }
        }
        
        // load cell_buf with current cell
        for (char i=sizeof(cell_buf); i > 0; i--) {
            ((byte_ptr_t)&cell_buf)[0] = NVMEM_READ_BYTE((byte_ptr_t)cellptr);
        }

        if (CELL_IS_INTEGER(cell_buf)) {
            context = nvmem_cell_visit_integer(bs, &cell_buf, context);

            cellptr = (CELL*)((byte_ptr_t)cellptr + sizeof(CELL));
            current_frame->counter--;
        
        } else if (CELL_IS_SYMBOL(cell_buf)) {
            context = nvmem_cell_visit_symbol(bs, &cell_buf, context);
            dassert(!CELL_SYMBOL_IS_PTR(cell_buf), CELL_SYMBOL_PTR_IN_NVMEM);
            cell_buf.header.Symbol.is_ptr = TRUE;
            cell_buf.string_ptr = nvmem_addr_to_chrptr(
                (nvmem_ptr_t)cellptr + sizeof(CELLHEADER));

            cellptr = (CELL*)(
                (byte_ptr_t)cellptr + CELL_SYMBOL_LENGTH(cell_buf));

            current_frame->counter--;

        } else if (CELL_IS_LIST(cell_buf)) {
            context = nvmem_cell_enter_list(bs, &cell_buf, context);
            dassert(!CELL_LIST_IS_PTR(cell_buf), CELL_LIST_PTR_IN_NVMEM);

            struct frame *new_frame = (struct frame*)(
                bistack_heapalloc(bs, sizeof(struct frame)));

            new_frame->counter = CELL_LIST_LENGTH(cell_buf);
            new_frame->next = current_frame;
            memcpy(&new_frame->cell_buf, &cell_buf, sizeof(cell_buf));

            cellptr = (CELL*)((byte_ptr_t)cellptr + sizeof(CELLHEADER));

            current_frame->counter--;
            current_frame = new_frame;
        } else {
            dassert(FALSE, UNEXPECTED_BRANCH);
        }
    }

    const void *rewind_mark = bistack_heaprewind(bs);
    dassert(rewind_mark == mark, 0);

    return cellptr;
}

struct _nvmem_cell_advance_context {
    nvmem_ptr_t endpos;
};

void *nvmem_cell_advance_list_leave_callback(
        BISTACK *bs, CELL *cellptr, void *context) {
    ((struct _nvmem_cell_advance_context *)context)->endpos = 0;
}

nvmem_ptr_t nvmem_cell_advance(BISTACK *bs, CELL *cellptr) {
    struct _nvmem_cell_advance_context context;   
    nvmem_cell_iterate(
        bs, 
        cellptr, 
        cell_visit_pass,
        nvmem_cell_advance_list_leave_callback,
        cell_visit_pass,
        cell_visit_pass,
        &context
        );
    return context.endpos;
}

CELL *cell_advance(BISTACK *bs, CELL *cellptr) {
    /**
     * Advances to next cell.
     */
    CELLHEADER cellheader_buf;
    cellheader_load((CELLHEADER*)cellptr, &cellheader_buf);

    dassert(CELLHEADER_IS_LIST(cellheader_buf), RUNTIME_LIST_EXPECTED);
    
    const void *mark = bistack_heapmark(bs);
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

    const void *rewind_mark = bistack_heaprewind(bs);
    dassert(rewind_mark == mark, 0);

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
