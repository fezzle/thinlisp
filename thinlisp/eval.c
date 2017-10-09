#include "eval.h"
#include "defines.h"
#include "environment.h"
#include "runtime.h"
#include "utils.h"


typedef struct frame {
    void *bistack_mark;
    struct frame *next;
    CELL *cell;
    uint8_t is_quoted:1;
} FRAME;

typedef CELLHEADER *(*native_method)(ENVIRONMENT *, CELLHEADER *);

CELLHEADER *plus(ENVIRONMENT *environment, CELLHEADER *cell);
CELLHEADER *minus(ENVIRONMENT *environment, CELLHEADER *cell);
CELLHEADER *cond(ENVIRONMENT *environment, CELLHEADER *cell);
CELLHEADER *less_than(ENVIRONMENT *environment, CELLHEADER *cell);
CELLHEADER *less_than_or_equal(ENVIRONMENT *environment, CELLHEADER *cell);
CELLHEADER *greater_than(ENVIRONMENT *environment, CELLHEADER *cell);
CELLHEADER *greater_than_or_equal(ENVIRONMENT *environment, CELLHEADER *cell);
CELLHEADER *equal(ENVIRONMENT *environment, CELLHEADER *cell);
CELLHEADER *first(ENVIRONMENT *environment, CELLHEADER *cell);
CELLHEADER *rest(ENVIRONMENT *environment, CELLHEADER *cell);
CELLHEADER *evaluate(ENVIRONMENT *environment, CELLHEADER *cell);


native_method get_native_method(ENVIRONMENT *environment, CELL *symbolcell) {
    uint8_t initialized = FALSE;
    static struct native_method_mapping {
        CELL cell;
        native_method method;
    } native_methods[] = {
        {
            .cell = {
                .header.Symbol.type = AST_SYMBOL,
                .header.Symbol.prefix = AST_NONE,
                .header.Symbol.length = 1,
                .header.Symbol.is_ptr = TRUE,
                .string_ptr = PSTR("+"),
            },
            .method = plus,
        },
        {
            .cell = {
                .header.Symbol.type = AST_SYMBOL,
                .header.Symbol.prefix = AST_NONE,
                .header.Symbol.length = 1,
                .header.Symbol.is_ptr = TRUE,
                .string_ptr = PSTR("-"),
            },
            .method = minus,
        },
        {
            .cell = {
                .header.Symbol.type = AST_SYMBOL,
                .header.Symbol.prefix = AST_NONE,
                .header.Symbol.length = 4,
                .header.Symbol.is_ptr = TRUE,
                .string_ptr = PSTR("cond"),
            },
            .method = cond,
        },
        {
            .cell = {
                .header.Symbol.type = AST_SYMBOL,
                .header.Symbol.prefix = AST_NONE,
                .header.Symbol.length = 1,
                .header.Symbol.is_ptr = TRUE,
                .string_ptr = PSTR("<"),
            },
            .method = less_than,
        },
        {
            .cell = {
                .header.Symbol.type = AST_SYMBOL,
                .header.Symbol.prefix = AST_NONE,
                .header.Symbol.length = 2,
                .header.Symbol.is_ptr = TRUE,
                .string_ptr = PSTR("<="),
            },
            .method = less_than_or_equal,
        },
        {
            .cell = {
                .header.Symbol.type = AST_SYMBOL,
                .header.Symbol.prefix = AST_NONE,
                .header.Symbol.length = 1,
                .header.Symbol.is_ptr = TRUE,
                .string_ptr = PSTR("="),
            },
            .method = equal,
        },
        {
            .cell = {
                .header.Symbol.type = AST_SYMBOL,
                .header.Symbol.prefix = AST_NONE,
                .header.Symbol.length = 1,
                .header.Symbol.is_ptr = TRUE,
                .string_ptr = PSTR(">"),
            },
            .method = greater_than,
        },
        {
            .cell = {
                .header.Symbol.type = AST_SYMBOL,
                .header.Symbol.prefix = AST_NONE,
                .header.Symbol.length = 2,
                .header.Symbol.is_ptr = TRUE,
                .string_ptr = PSTR(">="),
            },
            .method = greater_than_or_equal,
        },
        {
            .cell = {
                .header.Symbol.type = AST_SYMBOL,
                .header.Symbol.prefix = AST_NONE,
                .header.Symbol.length = 5,
                .header.Symbol.is_ptr = TRUE,
                .string_ptr = PSTR("first"),
            },
            .method = first,
        },
        {
            .cell = {
                .header.Symbol.type = AST_SYMBOL,
                .header.Symbol.prefix = AST_NONE,
                .header.Symbol.length = 4,
                .header.Symbol.is_ptr = TRUE,
                .string_ptr = PSTR("rest"),
            },
            .method = rest,
        },
        {
            .cell = {
                .header.Symbol.type = AST_SYMBOL,
                .header.Symbol.prefix = AST_NONE,
                .header.Symbol.length = 4,
                .header.Symbol.is_ptr = TRUE,
                .string_ptr = PSTR("eval"),
            },
            .method = evaluate,
        }
    };
    const list_size_t native_method_count = (
        sizeof(native_methods) / sizeof(struct native_method_mapping));

    if (!initialized) {
        for (list_size_t i=0; i<native_method_count; i++) {
            uint8_t hash = hashstr_8_P(
                native_methods[i].cell.string_ptr,
                native_methods[i].cell.header.Symbol.length);
            native_methods[i].cell.header.Symbol.hash = hash & SYMBOL_HASH_MASK;
        }
        initialized = TRUE;
    }

    lassert(CELL_IS_SYMBOL(symbolcell->header), RUNTIME_SYMBOL_EXPECTED);

    for (list_size_t i=0; i<native_method_count; i++) {
        if (native_methods[i].cell.header.Symbol.hash !=
                symbolcell->header.Symbol.hash) {
            // hash doesnt match, continue
            continue;
        }
        if (!cell_symbol_compare(symbolcell, &native_methods[i].cell)) {
            // do a string comparison
            return native_methods[i].method;
        }
    }
}

CELL *push_symbol_cell(ENVIRONMENT *environment, CELL *cell) {
    CELL *copied = bistack_push(environment->bs, sizeof(CELL));
    copied->header.Symbol.type = AST_SYMBOL;
    copied->header.Symbol.prefix = cell->header.Symbol.prefix;
    copied->header.Symbol.length = cell->header.Symbol.length;
    copied->header.Symbol.hash = cell->header.Symbol.hash;
    copied->header.Symbol.is_ptr = TRUE;
    copied->string_ptr = (
        cell->header.Symbol.is_ptr ? cell->string_ptr : cell->string);
    return copied;
}


CELL *env_push_resolved_symbol(ENVIRONMENT *env, CELL *symbol_cell) {
    env->
}

CELL *evaluate(ENVIRONMENT *environment, CELL *cell) {
    FRAME *parentframe = NULL;
    uint8_t comma_count = 0;

    while (TRUE) {
        FRAME *cellframe = bistack_allocb(environment->bs, sizeof(FRAME));
        cellframe->cell = cell;
        cellframe->next = parentframe;

        if (CELL_IS_SYMBOL(cell->header)) {
            if (CELL_SYMBOL_PREFIX(cell->header) == AST_QUOTE) {
                push_symbol_cell(environment, cell);
            } else {
                env_push_resolved_symbol(environment, cell);

            }

        } else if (CELL_IS_LIST(*cell)) {
            if (CELL_LIST_PREFIX(*cell) == AST_QUOTE) {
                return cell;

            } else if (cell->List.length == 0) {
                return cell;

            } else {
                // evaluate
                list_size_t sexp_len = cell->List.length;
                CELLHEADER *sexp = cell_list_get_cells(cell);
                CELLHEADER *method = sexp;

                CELL rest;
                cell_list_init(&rest.header, AST_NONE, TRUE, sexp_len - 1);
                rest.cells_ptr = sexp + 1;

                native_method fn = get_native_method(environment, method);
                if (fn) {
                    CELLHEADER *res= fn(environment, rest);
                }
            }
        }
    }
}




inline CELLHEADER *cell_next_cell(
        ENVIRONMENT *environment, CELLHEADER *cellheader) {
    struct frame {
        struct frame *up;
        list_size_t cell_counter;
    };
    struct frame *up = &(struct frame){.up = NULL, .cell_counter = 1};

    while (TRUE) {
        if (CELL_IS_LIST(*cellheader)) {
            struct frame *newframe = bistack_alloc(
                environment->bs, sizeof(struct frame));
            newframe->up = up;
            newframe->cell_counter = cellheader->List.length;
            up = newframe;
        } else if (CELL_IS_SYMBOL(*cellheader)) {
            return (CELLHEADER*)(
                (void*)cellheader +
                sizeof(CELLHEADER) +
                CELL_SYMBOL_LENGTH(*cellheader));
        } else {
            return cellheader + 1;
        }
    }
}

CELLHEADER *first(ENVIRONMENT *environment, CELLHEADER *args) {
    lassert(args->List.type == AST_LIST, RUNTIME_LIST_EXPECTED);
    if (args->List.length == 0) {
        return CELL_FALSE;
    } else {
        return cell_list_cells(args);
    }
}

CELLHEADER *rest(ENVIRONMENT *environment, CELLHEADER *args) {
    lassert(args->List.type == AST_LIST, RUNTIME_LIST_EXPECTED);
    if (args->List.length == 0) {
        return CELL_FALSE;
    } else {
        CELLHEADER *res = bistack_alloc(environment->bs, sizeof(CELLHEADER));
        cell_list_init(res, args->List.prefix, TRUE, args->List.length - 1);
        ((CELL*)res)->cells_ptr = args + 2;
    }
}


CELLHEADER *lists_equal() {
    while (length--) {
        CELLHEADER *res = equal(bs, list1, list2);
        if (CELL_IS_FALSE(*res)) {
            return res;
        }
    }
    return CELL_TRUE;
}

CELLHEADER *strings_equal(
        ENVIRONMENT *environment,
        char *str1,
        char *str2,
        string_size_t length) {
    while (length--) {
        if (*str1++ != *str2++) {
            return CELL_FALSE;
        }
    }
    return CELL_TRUE;
}

CELLHEADER *equal(ENVIRONMENT *e, CELLHEADER *cell) {
    CELLHEADER *subcells = cell_list_cells(cell);
    CELLHEADER *first = subcells + 1;
    CELLHEADER *second = subcells + 2;
    if (first->wholeheader != second->wholeheader) {
        return CELL_FALSE;
    }
    if (CELL_IS_LIST(*first)) {

    } else if (CELL_IS_SYMBOL(*first)) {
        return strings_equal(
            bs, (char*)first+1, (char*)second+1, CELL_SYMBOL_LENGTH(*first));
    } else {
        // integer vals are contained in header
        return CELL_TRUE;
    }
}

CELLHEADER *subtract(ENVIRONMENT *environment, list_size_t argc, CELLHEADER **argv) {
    int16_t value = 0;
    for (; argc--; argv++) {
        lassert(CELL_IS_INTEGER(**argv), RUNTIME_INTEGER_EXPECTED);
        value -= CELL_INTEGER_VAL(**argv);
    }
    return make_integer(bs, value);
}

CELLHEADER *add(ENVIRONMENT *environment, list_size_t argc, CELLHEADER **argv) {
    int16_t value = 0;
    for (; argc--; argv++) {
        lassert(CELL_IS_INTEGER(**argv), RUNTIME_INTEGER_EXPECTED);
        value += CELL_INTEGER_VAL(**argv);
    }
    return make_integer(bs, value);
}
