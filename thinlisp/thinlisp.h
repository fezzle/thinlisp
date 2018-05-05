#ifndef THINLISP_H
#define THINLISP_H

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <assert.h>
#include <alloca.h>
#include <stdio.h>
#include <string.h>

// non harvard defines
#define strncmp_P strncmp


#define PSTR(X) (X)
#define DEBUG 1
#define TRUE (1)
#define FALSE (0)
#define MAX_RECURSION 8

jmp_buf __jmpbuff;

void lerror(uint16_t exctype, char *err, ...) {
  longjmp(__jmpbuff, exctype);
}


void lassert(uint16_t truefalse, uint16_t exctype, ...) {
    if (!truefalse) {
        lerror(exctype, PSTR("ASSERTION FAILED"));
    }
}

void dassert(uint16_t truefalse, uint16_t exctype, ...) {
#ifdef DEBUG
    if (!truefalse) {
        lerror(exctype, PSTR("DEBUG ASSERTION FAILED"));
    }
#endif
}

#define CELL_LIST_NODE_COUNT 3

enum { INTEGER=0, BIGINTEGER=1, STRING=2, LIST=3 };
enum {
    MEM_OUT_OF_IT=1,
    MEM_LOCKED,
    MEM_UNLOCKED,

    INTEGER_UNPACK_ON_INCORRECT_TYPE=10,
    BINDING_NODE_SIZE_REMAINING_NONZERO,
    RECURSION_DEPTH_EXCEEDED,
    LIST_FIRST_DIDNT_GET_LIST,
    LIST_APPEND_TO_NOT_LIST,
    LIST_APPEND_LIST_CELL_BADSTATE,
    LIST_NTH_OUT_OF_BOUNDS,
    BIND_NOT_LIST,
    BIND_SYMBOL_LIST_ODD_LENGTH,

    READER_UNGETC_ERROR=20,

    INVALID_ARG_COUNT=40,

    SYMBOL_EVAL_NOT_STRING=50,
};

typedef struct mem {
    void *start;
    void *end;

    void *bottom;
    void **mark;

    uint8_t locked:1;
} MEM;




typedef struct cell_list_node CELL_LIST_NODE;
typedef struct cell {
    uint8_t type:2;
    uint8_t length:6;
    union {
        struct {
            union {
                // strings are read backwards, then switched to forwards.
                char *rev_str;
                char *str;
            };
        };
        struct {
            uint16_t integer;
        };
        struct {
            CELL_LIST_NODE *list_node;
        };
        struct {
            struct cell *next;
        };
    };
} CELL;

typedef struct cell_list_node {
    union {
        struct {
            CELL *cell0;
            CELL *cell1;
            CELL *last_cell;
        };
        struct {
            CELL *symbol;
            CELL *value;
            CELL_LIST_NODE *next;
        };
        struct {
            CELL *cells[CELL_LIST_NODE_COUNT];
        };
        struct {
            CELL_LIST_NODE *trie_cells_with_next[CELL_LIST_NODE_COUNT - 1];
            CELL_LIST_NODE *trie_next;
        };
        struct {
            CELL_LIST_NODE *trie_cells[3];
        };
    };
} CELL_LIST_NODE;


typedef CELL* (*builtin_fn)(MEM *const m, CELL *list);

builtin_fn add;
builtin_fn subtract;
builtin_fn eq;
builtin_fn defn;
builtin_fn cond;
builtin_fn iff;
builtin_fn let;
builtin_fn append;

enum modifier {
    QUOTE, LAZY_EVAL,
};

typedef struct builtin_binding {
    builtin_fn fn;
    uint8_t modifier:2;
    uint8_t symbol_length:6;
    char *symbol;
} BUILTIN_BINDINGS[] = {
    { .fn = add, .modifier = 0, .symbol = "+" },
    { .fn = subtract, .modifier = 0, .symbol = "-"},
    { .fn = eq, .modifier = 0, .symbol = "="},
    { .fn = defn, .modifier = QUOTE, .symbol_length .symbol = "defn"},
    { .fn = cond, .modifier = LAZY_EVAL, .symbol_length = 4, .symbol = "cond"},
    { .fn = iff, .modifier = LAZY_EVAL, .symbol_length = 3, .symbol = "if"},
    { .fn = let, .modifier = QUOTE, .symbol_length = 3, .symbol = "let"},
    { .fn = append, .modifier = 0, .symbol_length = 6, .symbol = "append"},
};

builtin_fn builtin_fn_lookup(CELL *const symbol) {
    dassert(symbol->type == STRING, SYMBOL_EVAL_NOT_STRING);
    switch(symbol->length) {
        case 1:
            const char c = symbol->str[0];
            if (c == '+') {
                return add;
            } else if (c == '-') {
                return subtract;
            } else if (c == '=') {
                return eq;
            }
            break;
        case 2:
            const char c0 = symbol->str[0];
            const char c1 = symbol->str[1];
            if (c0 == 'i' && c1 == 'q') {
                return iff;
            } else if (c0 == 'e' && c1 == 'q') {
                return iff;
            }
            break;
        default:
            char *const str = symbol->str;
            const uint8_t len = symbol->length;
            if (strncmp(str, PSTR("defn"), len) == 0) {
                return defn;
            } else if (strncmp(str, PSTR("cond"), len) == 0) {
                return cond;
            } else if (strncmp(str, "let", len) == 0) {
                return let;
            }
            break;
    }
    return NULL;
}


#define FN_BUILTIN_COUNT \
    (sizeof(BUILTIN_BINDINGS) / sizeof(struct builtin_binding))

typedef struct reader_state {
    uint8_t parse_state;
    union {
        struct {
            char first_char;
            char last_char;
        };
        struct {
            // int reading
            uint32_t accumulator;
            int8_t sign;
        };
        CELL *cell;
    };
    struct reader_state *parent;
} READER_STATE;

 /* BINDINGS */
typedef struct binding_pair {
    CELL *symbol;
    CELL *value;
} BINDING_PAIR;

typedef struct binding_vlist_node {
    struct binding_vlist_node *next;
    BINDING_PAIR pairs[1];
} BINDING_VLIST_NODE;

typedef struct binding_mark {
    uint8_t size;
    struct binding_mark *parent;
} BINDING_MARK;

#define SIZEOF_BINDING_VLIST_NODE(N) \
    (sizeof(BINDING_VLIST_NODE) + sizeof(BINDING_PAIR) * ((N) - 1))

typedef struct binding_vlist {
    BINDING_MARK *marks;
    BINDING_VLIST_NODE *root;
    uint8_t size;
} BINDING_VLIST;

typedef struct symbol_binding {
    CELL *symbol_cell;
    CELL *bound_cell;
    struct symbol_binding *next;
} SYMBOL_BINDING;

typedef struct symbol_binding_frame {
    SYMBOL_BINDING *symbol_binding;
    struct symbol_binding_frame *next;  
} SYMBOL_BINDING_FRAME;



#endif