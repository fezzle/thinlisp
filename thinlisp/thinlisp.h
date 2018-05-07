#ifndef THINLISP_H
#define THINLISP_H

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <assert.h>
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
    MEM_NOT_TWO_BYTE_ALIGNED,

    INTEGER_UNPACK_ON_INCORRECT_TYPE=10,
    BUILTIN_HASH_COLLISION,
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

    LENGTH_INVALID_NUMBER_OF_ARGUMENTS,
    LENGTH_FIRST_ARG_MUST_BE_LIST,
    MAP_INVALID_NUMBER_OF_ARGUMENTS,
    MAP_FIRST_ARG_MUST_BE_EXPR,
    MAP_SECOND_ARG_MUST_BE_LIST,
};

typedef struct mem {
    void *start;
    void *end;

    void *bottom;
    void **mark;

    uint8_t locked:1;
} MEM;

typedef struct cell {
    union {
        struct {
            uint32_t type:2;
            uint32_t length:6;
            uint32_t hash:4;
            uint32_t str_ptr:10;
            uint32_t next_ptr:10;
        };
        struct {
            uint32_t int_type:2;
            uint32_t int_val:20;  
            uint32_t int_next_ptr:10;
        };
        struct {
            uint32_t list_type:2;
            uint32_t list_tail_ptr:10;
            uint32_t list_head_ptr:10;
            uint32_t list_next_ptr:10;
        };
        struct {
            uint32_t pair_type:2;
            uint32_t pair_first:10;
            uint32_t pair_second:10;
            uint32_t pair_next_ptr:10;            
        };
    };
} CELL;

CELL *unpack_ptr(MEM *const m, uint16_t ptr) {
    if (ptr == 0) {
        return NULL;
    }
    return (void*)((ptrdiff_t)m->end - (2<<10) + (ptr << 1));
}

uint16_t pack_ptr(MEM *const m, CELL *const ptr) {
    lassert(((ptrdiff_t)ptr & 1) == 0, MEM_NOT_TWO_BYTE_ALIGNED);
    if (ptr == NULL) {
        return 0;
    }
    return (uint16_t)(((ptrdiff_t)ptr - ((ptrdiff_t)m->end - (2<<10))) >> 1);
}

typedef CELL* (*builtin_fn)(MEM *const m, CELL *list);


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


typedef struct reader {
    void *getc_streamobj;
    char (*getc)(void *getc_streamobj);

    void *putc_streamobj;
    char (*putc)(void *putc_streamobj, char c);

    char ungetbuff[4];

    uint8_t ungetbuff_i : 2;

    MEM *mem;

    CELL *root;
    CELL *cell_free_list;

    READER_STATE *state;
    READER_STATE *unused_states;

    BINDING_VLIST *bindings;
} READER;


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