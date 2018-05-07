//
//  main.c
//  thinlisp
//
//  Created by Eric on 2016-09-11.
//  Copyright © 2016 norg. All rights reserved.
//

#define WINDOWS

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <assert.h>
#include <alloca.h>
#include <stdio.h>
<<<<<<< Updated upstream
#include "thinlisp.h"
=======
#ifdef WINDOWS
    #include <malloc.h>
    #define alloca _alloca
#else
    #include <alloca.h>
#endif

#define PSTR(X) (X)
#define DEBUG 1
#define TRUE (1)
#define FALSE (0)
#define MAX_RECURSION 8
#define VLIST_NODE_INITIAL_SIZE (2)


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

enum { INTEGER=0, BIGINTEGER=1, STRING=2, LIST=3 };
enum {
    MEM_OUT_OF_IT=1,
    MEM_LOCKED,
    MEM_UNLOCKED,

    INTEGER_UNPACK_ON_INCORRECT_TYPE=10,
    BINDING_NODE_SIZE_REMAINING_NONZERO,
    RECURSION_DEPTH_EXCEEDED,
    LIST_FIRST_DIDNT_GET_LIST,
    VLIST_INDEX_OUT_OF_BOUNDS,

    READER_UNGETC_ERROR=20,

    INVALID_ARG_COUNT=40,
};

typedef struct mem {
    void *start;
    void *end;

    void *bottom;
    void **mark;

    uint8_t locked:1;
} MEM;
>>>>>>> Stashed changes

MEM *mem_malloc(uint16_t size) {
    MEM *mem = malloc(size);
    mem->start = mem + sizeof(MEM);
    mem->end = ((char*)mem) + size;

    // set last bottom mark to NULL
    mem->bottom = ((char*)mem->end)[-sizeof(void*)];
    mem->mark = mem->bottom; 
    *mem->mark = NULL; //mem->bottom;
    mem->locked = FALSE;

    return mem;
}
void mem_unlock(MEM *const m, void *new_bottom) {
    lassert(m->locked, MEM_UNLOCKED);
    lassert(m->start <= new_bottom, MEM_OUT_OF_IT);
    m->locked = FALSE;
    m->bottom = new_bottom;
}
void *mem_alloc(MEM *const m, uint16_t size) {
    lassert(!m->locked, MEM_LOCKED);
    m->bottom = &((char*)m->bottom)[-size];
    lassert(m->start <= m->bottom, MEM_OUT_OF_IT);
    return m->bottom;
}
void mem_mark(MEM *const m) {
    lassert(!m->locked, MEM_LOCKED);
    m->bottom = &((char*)m->bottom)[-sizeof(void*)];
    *((void**)m->bottom) = m->mark;
    m->mark = &m->bottom;
}
void mem_rewind(MEM *const m) {
    lassert(!m->locked, MEM_LOCKED);
    m->mark = *m->mark;
    m->bottom = m->mark;    
}

<<<<<<< Updated upstream
typedef struct cell_list_node CELL_LIST_NODE;
=======
struct status {
    uint8_t overflows;
    uint8_t underflows;
} STATUS;

>>>>>>> Stashed changes
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

<<<<<<< Updated upstream
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

CELL *list_append(MEM *m, CELL *l, CELL *c) {
    dassert(l->type == LIST, LIST_APPEND_TO_NOT_LIST);
    if (c->list_node == NULL) {
        c->list_node = mem_alloc(m, sizeof(CELL_LIST_NODE));
    }
    CELL_LIST_NODE *itr = c->list_node;
    uint8_t length_remaining = c->length;
    while (TRUE) {
        if (length_remaining > CELL_LIST_NODE_COUNT) {
            length_remaining = length_remaining - (CELL_LIST_NODE_COUNT - 1);
            itr = itr->next;
        } else if (length_remaining == CELL_LIST_NODE_COUNT) {
            // must allocate a new one and move last cell in current forward
            //  so `next` can be used
            CELL *t = itr->last_cell;
            itr->next = mem_alloc(m, sizeof(CELL_LIST_NODE));
            itr = itr->next;
            itr->cells[0] = t;
            itr->cells[1] = c;
            break;
        } else {
            itr->cells[length_remaining - 1] = c;
        }
    }
    c->length++;
    return l;
}

CELL *list_nth(MEM *m, CELL *l, uint8_t n) {
    dassert(n < l->length, LIST_NTH_OUT_OF_BOUNDS);
    uint8_t length_remaining = n;
    CELL_LIST_NODE *itr = l->list_node;
    while (TRUE) {
        if (length_remaining > CELL_LIST_NODE_COUNT) {
            length_remaining = length_remaining - (CELL_LIST_NODE_COUNT - 1);
            itr = itr->next;
        } else {
            return itr->cells[length_remaining - 1];
        }
    }
}

CELL *list_slice(MEM *m, CELL *l, uint8_t start, uint8_t end) {
    dassert(end < l->length, LIST_NTH_OUT_OF_BOUNDS);
    
    CELL *new_list = mem_alloc(m, sizeof(CELL));
    new_list->type = LIST;
    new_list->length = end - start;
    if (new_list->length == 0) {
        return new_list;
    }
    new_list->list_node = mem_alloc(m, sizeof(CELL_LIST_NODE));
            
    uint8_t length_remaining = start;
    CELL_LIST_NODE *itr = l->list_node;
    while (TRUE) {
        if (length_remaining > CELL_LIST_NODE_COUNT) {
            length_remaining = length_remaining - (CELL_LIST_NODE_COUNT - 1);
            itr = itr->next;
        } else {
            if (length_remaining == 1) {
                new_list->list_node->cells[0] = itr->cells[length_remaining - 1];
                new_list->list_node->cells[1] = itr->cells[length_remaining];
                new_list->list_node->next = itr->next;
            } else if (length_remaining == 2) {
                new_list->list_node->cells[0] = NULL;
                new_list->list_node->cells[1] = itr->cells[length_remaining];
                new_list->list_node->next = itr->next;
            } else if (length_remaining == 3) {
                new_list->list_node->cells[0] = NULL;
            }
        }
    }
}

CELL *bind(
        MEM *const m,
        CELL *const symbol_list,
        CELL *const symbol,
        CELL *const value) {
    dassert(symbol_list->type == LIST, BIND_NOT_LIST);
    dassert((symbol_list->length & 1) == 0, BIND_SYMBOL_LIST_ODD_LENGTH);
    CELL_LIST_NODE *new_bind = mem_alloc(m, sizeof(CELL_LIST_NODE));
    new_bind->next = symbol_list->list_node;
    symbol_list->list_node = new_bind;
    symbol_list->length += 2;
    return symbol_list;
=======
typedef struct vlist_node {
    union {
        // these ptr arrays must be same size elements (expecting sizeof(ptr))
        // only cells is used unless there is a next entry in the last position
        void *cells[VLIST_NODE_INITIAL_SIZE];
        struct vlist_node *next[VLIST_NODE_INITIAL_SIZE];
    };
} VLIST_NODE;

typedef struct vlist {
    uint8_t size;
    VLIST_NODE *root;
} VLIST;

VLIST *cell_vlist_new(MEM *const m) {
    VLIST *cv = mem_alloc(m, sizeof(VLIST));
    cv->root = mem_alloc(m, sizeof(VLIST_NODE));
    cv->size = 0;
}

void vlist_append(MEM *const m, VLIST *const cv, void *const c) {
    uint8_t cur_node_size = VLIST_NODE_INITIAL_SIZE;
    uint8_t size_rem = cv->size;
    VLIST_NODE *node_ptr = cv->root;
    while (TRUE) {
        size_rem = size_rem - cur_node_size;
        if (size_rem <= 0) {
            break;
        } else {
            // there's more nodes in the list and the last element was used 
            // for next instead of cell
            size_rem++;
        }
        node_ptr = node_ptr->next[cur_node_size-1];
        cur_node_size <<= 1;
    }
    if (size_rem == 0) {
        // last list full, allocate new
        CELL *tmp = node_ptr->cells[cur_node_size-1];
        node_ptr->next[cur_node_size-1] = mem_alloc(m, cur_node_size<<1);
        node_ptr->next[cur_node_size-1]->cells[0] = tmp;
        node_ptr->next[cur_node_size-1]->cells[1] = c;
    } else {
        node_ptr->cells[size_rem + cur_node_size] = c;
    }
    cv->size++;
}

void *vlist_nth(VLIST *const cv, const uint8_t nth) {
    lassert(nth >= cv->size, VLIST_INDEX_OUT_OF_BOUNDS);
    uint8_t cur_node_size = VLIST_NODE_INITIAL_SIZE;
    uint8_t size_rem = nth;
    VLIST_NODE *node_ptr = cv->root;
    while (TRUE) {
        size_rem = size_rem - cur_node_size;
        if (size_rem <= 0) {
            break;
        } else {
            // more nodes in the list, last element is next ptr
            size_rem++;
        }
        node_ptr = node_ptr->next[cur_node_size-1];
        cur_node_size <<= 1;
    }
    return node_ptr->cells[size_rem + cur_node_size];
}

int32_t cell_integer_unpack(CELL *const cell) {
    dassert(cell->type == INTEGER, INTEGER_UNPACK_ON_INCORRECT_TYPE);
    int32_t res = (cell->length << 16) + cell->integer;
    if (cell->length & 0x20) {
        res = (-res) + 1;
    }
    return res;
}

CELL *cell_integer_pack(CELL *const cell, int32_t val) {
    #define MAX_INT (1<<(16+5) - 1)
    #define MIN_INT (-1<<(16+5))
    if (val > MAX_INT) {
        val = MAX_INT;
        STATUS.overflows++;
    } else if (val < MIN_INT) {
        val = MIN_INT;
        STATUS.underflows++;
    }
    // integer should be 16bits, store extra bits in length
    cell->length = (val >> 16) & 0x3F;
    cell->integer = (uint16_t)val;
    return cell;
}

CELL *add(MEM *const m, CELL *const target, CELL *const list) {
    int32_t val = 0;
    for (uint8_t n=1; n<list->length; n++) {
        val += cell_integer_unpack(&list[n]);
    }
    return cell_integer_pack(target, val);
}

CELL *subtract(MEM *const m, CELL *const target, CELL *const list) {
    CELL *c = mem_alloc(m, sizeof(CELL));
    int32_t val = 0;
    if (list->length >= 1) {
        val = cell_integer_unpack(list_nth(list, 1));
    }
    for (uint8_t n=2; n<list->length; n++) {
        val -= cell_integer_unpack(list_nth(list, n));
    }
    return cell_integer_pack(target, val);
}

CELL *list_first(CELL *const list) {
    lassert(list->type == LIST, LIST_FIRST_DIDNT_GET_LIST);
    return &list[1];
}

CELL *next_cell(CELL *const cell) {
    CELL *itr = cell;
    uint8_t remaining[MAX_RECURSION];
    uint8_t depth = 0;
    if (cell->type != LIST) {
        return &cell[1];
    }
    remaining[depth] = cell->length;
    itr++;
    while (TRUE) {
        if (remaining[depth] == 0) {
            depth--;
        }
        if (depth < 0) {
            break;
        }
        if (cell->type == LIST) {
            remaining[++depth] = cell->length;
            lassert(depth < MAX_RECURSION, RECURSION_DEPTH_EXCEEDED);
        }
        itr++;
    }
    return itr;
}


CELL *cond(MEM *const m, CELL *const list) {
    CELL *cond_counter = list_first(list);
    for (uint8_t n=1; n<list->length; n++) {
        CELL *predicate = eval(m, cond_counter);
        CELL *result = next_cell(predicate);
        if (predicate->integer) {
            return eval(m, result);
        }
        cond_counter = next_cell(result);
    }
    return FALSE;
}

CELL *iff(MEM *const mem, CELL *const list) {
    if (is_true(eval(mem, list[1]))) {
        return eval(mem, list[2]);
    } else if (list->length == 4) {
        return eval(mem, list[3]);
    }
}

CELL *let(MEM *const mem, CELL *const list) {
    return NULL;
}

CELL *append(MEM *const mem, CELL *const list) {

}

CELL *eq(MEM *const mem, CELL *const list) {
    lassert(list->length == 3, INVALID_ARG_COUNT);
    uint8_t remaining[MAX_RECURSION];
    uint8_t depth = 0;

    uint8_t val = 1;
    CELL *a = list_first(list);
    CELL *b = next_cell(a);
    while (TRUE) {
        if (a->type != b->type) {
            return FALSE;
        }
        if (a->length != b->length) {
            return FALSE;
        }
        if (a->type == INTEGER && a->integer != b->integer) {
            return FALSE;
        } else if (a->type == STRING) {
            for (uint8_t n=0; n<a->length; n++) {
                if (a->str[n] != b->str[n]) {
                    return FALSE;
                }
            }
        } else {
            remaining[depth++] = a->length;
        }
        a = next_cell(a);
        b = next_cell(b);
    }
}


#define BUILTIN_COUNT (sizeof(BUILTIN_BINDINGS) / sizeof(struct builtin_binding))

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


BINDING_VLIST *bind_new(MEM *const m, CELL *symbol, CELL *const value) {
    BINDING_VLIST *bv = mem_bottomalloc(m, sizeof(BINDING_VLIST));
BINDING_VLIST *bind_new(MEM *const m, CELL *const symbol, CELL *const value) {
    BINDING_VLIST *bv = mem_alloc(m, sizeof(BINDING_VLIST));
>>>>>>> Stashed changes
    bv->size = 0;
    bv->root = NULL;
    bv->marks = NULL;
    return bv;
};

void binding_push_mark(MEM *const m, BINDING_VLIST *const bv) {
<<<<<<< Updated upstream
    BINDING_MARK *mark = mem_bottomalloc(m, sizeof(BINDING_MARK));
=======
    BINDING_MARK *mark = mem_alloc(m, sizeof(BINDING_MARK));
>>>>>>> Stashed changes
    mark->size = bv->size;
    mark->parent = bv->marks;
    bv->marks = mark;
}

void binding_pop_mark(MEM *mem, BINDING_VLIST *const bv) {
    bv->size = bv->marks->size;
    bv->marks = bv->marks->parent;
}

<<<<<<< Updated upstream
void bind(MEM *const m, BINDING_VLIST *const bv, CELL *const symbol, CELL *const value) {
=======
void bind(
        MEM *const m, 
        BINDING_VLIST *const bv, 
        CELL *const symbol, 
        CELL *const value) {
>>>>>>> Stashed changes
    uint8_t size_remaining = bv->size;
    uint8_t node_size = 1;
    BINDING_VLIST_NODE **node = &bv->root;
    while (size_remaining > node_size) {
        if (size_remaining > node_size) {
            size_remaining -= node_size;
            node_size = node_size << 1;
            node = &(*node)->next;
        }
    }
    if (*node == NULL) {
        lassert(size_remaining == 0, BINDING_NODE_SIZE_REMAINING_NONZERO);
        *node = mem_alloc(m, SIZEOF_BINDING_VLIST_NODE(node_size));
    }
    (*node)->pairs[size_remaining].symbol = symbol;
    (*node)->pairs[size_remaining].value = value;
    bv->size++;
};
/*  BINDING */

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

<<<<<<< Updated upstream

static inline char reader_ungetc(READER *const r, const char c) {
=======
static inline char reader_ungetc(READER *const r, char c) {
>>>>>>> Stashed changes
    lassert(r->ungetbuff_i < sizeof(r->ungetbuff), READER_UNGETC_ERROR);
    r->ungetbuff[r->ungetbuff_i++] = c;
    return c;
}

static inline char reader_getc(READER *const r) {
    char c;
    if (r->ungetbuff_i) {
        c = r->ungetbuff[--r->ungetbuff_i];
    } else {
        c = r->getc(r->getc_streamobj);
    }
    return c;
}

static inline char reader_peekc(READER *const r) {
    if (r->ungetbuff_i) {
        return r->ungetbuff[r->ungetbuff_i-1];
    } else {
        char c = reader_getc(r);
        return reader_ungetc(r, c);
    }
}

static inline void reader_set_getc(
        READER *const r,
        char (*getc)(void *),
        void *getc_streamobj) {
    r->getc = getc;
    r->getc_streamobj = getc_streamobj;
}

static inline void reader_set_putc(
        READER *const r,
        char (*putc)(void *, char),
        void *putc_streamobj) {
    r->putc = putc;
    r->putc_streamobj = putc_streamobj;
}

static inline char reader_putc(READER *const r, const char c) {
    return r->putc(r->putc_streamobj, c);
}

static inline char is_whitespace(const char c) {
    return c == '\n' || c == ' ' || c == '\t';
}

static inline char is_alpha(const char c) {
    return (('A' <= c && c <= 'Z') ||
            ('a' <= c && c <= 'z') ||
            ('0' <= c && c <= '9'));
}

static inline char is_standard_char(const char c) {
    return c >= '!' && c <= '~';
}

static inline char is_non_symbol(const char c) {
    return c != '\n' && c != '\t' && c != ' ' && c != ')' && c != '(';
}

static inline char next_non_ws(READER *const r) {
    /*
    * reads characters until a non-whitespace character is found.
    */
    char c = ' ';
    while (c != -1 && is_whitespace(c)) {
        c = reader_getc(r);
    }
    if (c != -1) {
        // rewind 1
        reader_ungetc(r, c);
    }
    return c;
}

READER *reader_new(MEM *const m) {
    READER *r = mem_alloc(m, sizeof(READER));

    r->ungetbuff_i = 0;

    r->state = NULL;
    r->unused_states = NULL;
    return r;
}

enum { READING_STRING, READING_INTEGER, READING_ANYTHING, };


void reader_push_state(READER *const r) {
    // pushes a new state onto the reader
    READER_STATE *state;
    if (r->unused_states) {
        state = r->unused_states;
        r->unused_states = state->parent;
    } else {
        state = mem_alloc(r->mem, sizeof(READER_STATE));
    }
    state->parent = r->state;
    state->parse_state = READING_ANYTHING;
    r->state = state;
}

void reader_pop_state(READER *const r) {
    // pops READER_STATE off current list and puts it back in unused.
    READER_STATE *unused_state = r->state;
    r->state = r->state->parent;
    unused_state->parent = r->unused_states;
    r->unused_states = unused_state;
}

void reader_string_start(READER *const r) {
    reader_push_cell(r);

    READER_STATE *state = r->state;
    state->parse_state = READING_STRING;
    state->first_char = '\0';
    state->last_char = '\0';

    CELL *cell = state->cell;
    cell->type = STRING;
    cell->rev_str = mem_bottomlock(r->mem);
    cell->length = 0;
}

void reader_string_read(READER *const r, char c) {
    // strings are read backwards
    READER_STATE *state = r->state;
    if (state->first_char == '\n') {
        state->first_char = c;
    }
    state->last_char = c;

    CELL *cell = state->cell;
    *(--cell->rev_str) = c;
    cell->length++;
}

CELL *eval_string(READER *const r, CELL *const cell) {
    return cell;
}

CELL *reader_string_end(READER *const r) {
    // reverse the reverse-read string
    char *start = r->state->cell->rev_str;
    char *end = &start[r->state->cell->length-1];
    while (start < end) {
        char t = *start;
        *start = *end;
        *end = t;
        start++;
        end--;
    }

    return eval_string(r, r->state->cell);

    mem_bottomunlock(r->mem, r->state->cell->rev_str);
}

void reader_integer_start(READER *const r) {
    reader_push_new_state(r);
    reader_push_cell(r);
    r->state->cell->type = INTEGER;
    r->state->parse_state = READING_INTEGER;
    r->state->accumulator = 0;
    r->state->sign = 1;
}

void reader_integer_read(READER *const r, const char c) {
    if (c == '-' && r->state->accumulator == 0) {
        r->state->sign = -1;
    } else {
        uint32_t accumulator = r->state->accumulator;
        accumulator = (accumulator<<3) + (accumulator<<2);
        accumulator += '0' - c;
        r->state->accumulator = accumulator;
    }
}

void reader_integer_end(READER *const r) {
    cell_integer_pack(r->state->cell, r->state->accumulator);
}


void reader_list_start(READER *const r) {
    r->state->parse_state = READING_ANYTHING;

    r->state->cell = reader_get_temp_cell(r->mem, sizeof(CELL));
    r->state->cell->type = LIST;
    r->state->cell->length = 0;
}

<<<<<<< Updated upstream
void reader_list_read_element(READER *const r, CELL *c) {
=======
void reader_list_read_element(READER *const r, CELL *const cell) {
>>>>>>> Stashed changes
    reader_pop_state(r);
}

void reader_list_end(READER *const r) {
    //
}

CELL *reader_get_temp_cell(READER *const r) {
    if (r->cell_free_list == NULL) {
        CELL *c = mem_alloc(r->mem, sizeof(CELL));
        return c;
    } else {
        CELL *c = r->cell_free_list;
        r->cell_free_list = r->cell_free_list->next;
        return c;
    }
}

CELL *reader_free_temp_cell(READER *const r, CELL *const c) {
    c->next = r->cell_free_list;
    r->cell_free_list = c;
    return c; 
}


void reader_read(READER *const r) {
    while (TRUE) {
        char c = reader_getc(r);
        if (c == -1) {
            break;
        }

        uint8_t parse_state;
        if (r->state == NULL) {
            parse_state = READING_ANYTHING;
        } else {
            parse_state = r->state->parse_state;
        }

        switch (parse_state) {
        case READING_ANYTHING: {
            if (c == '(') {
                reader_list_start(r);

            } else if (c == ')') {
                reader_list_end(r);

            } else if (c == '-' || c == '+' || (c >= '0' && c <= '9')) {
                reader_integer_start(r);
                reader_integer_read(r, c);

            } else if (is_standard_char(c)) {
                reader_string_start(r);
                reader_string_read(r, c);

            } else {
                printf("unknown list, integer or string char: %c\n", c);
            }
            break;
        }
        case READING_STRING: {
            const char first_char = r->state->first_char;
            const char prev_char = r->state->last_char;
            const char is_escaped = prev_char == '\\';

            if (!is_escaped && ((first_char == c) || is_whitespace(c))) {
                reader_string_read(r, c);
                reader_string_end(r);
            } else {
                reader_string_read(r, c);
            }
            break;
        }
        case READING_INTEGER:
            if (is_whitespace(c)) {
                reader_integer_end(r);
            } else if (c >= '0' && c <= '9') {
                reader_integer_read(r, c);
            } else {
                printf("unknown integer char: %c\n", c);
            }
            break;
        }
    }
}


//#define READER_MAIN_TEST
#ifdef READER_MAIN_TEST
#include <time.h>

typedef struct {
    READER *reader;
    char is_done;
    char last_char;
    char is_paused;
} INPUT_STREAM;

typedef struct {
    READER *reader;
} OUTPUT_STREAM;

char mygetc(void *streamobj) {
    INPUT_STREAM *is = (INPUT_STREAM*)streamobj;
    READER *reader = is->reader;

    if (is->is_paused == FALSE && is->last_char == '\n') {
        is->is_paused = TRUE;
        return -1;
    } else {
        is->is_paused = FALSE;
    }

    int c = getchar();
    is->last_char = c;
    if (c == -1) {
        is->is_done = TRUE;
        return -1;
    } else {
        return c;
    }
}

char myputc(void *streamobj, char c) {
    putchar(c);
    return c;
}

void sleep_15ms() {
    static struct timespec fifteen_millis = {.tv_sec=0, .tv_nsec=15000};
    nanosleep(&fifteen_millis, NULL);
}

int main(int argc, char **argv) {
    MEM *m = mem_malloc(16000);
    READER *reader = reader_new(m);

    INPUT_STREAM is = {
        .reader=reader,
        .is_done=FALSE,
        .last_char=' ',
        .is_paused=TRUE,
    };
    OUTPUT_STREAM os = {
        .reader=reader
    };

    int exctype = setjmp(__jmpbuff);
    if (exctype != 0) {
        printf("\n *** %d\n", exctype);
    }

    reader_init(reader);
    reader_set_getc(reader, mygetc, &is);
    reader_set_putc(reader, myputc, &os);
    is.is_paused = TRUE;
    is.last_char = ' ';

    while (!is.is_done) {
        putchar('>');
        putchar(' ');
        while (!reader_read(reader)) {
            if (is.last_char == '\n') {
                while(!reader_put_missing(reader)) {
                    sleep_15ms();
                }
                break;
            }
        }
        sleep_15ms();
    }
    return 0;
}

#else

char mygetc(void *s) {
    return (char)getchar();
}
char myputc(void *streamobj, char c) {
    return putchar(c);
}

int main(int argc, char **argv) {
    MEM *m = (MEM*)malloc(32000);
    READER *r = reader_new(m);

    reader_set_getc(r, mygetc, NULL);
    reader_set_putc(r, myputc, NULL);
    reader_read(r);
}
#endif
