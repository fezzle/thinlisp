//
//  main.c
//  thinlisp
//
//  Created by Eric on 2016-09-11.
//  Copyright © 2016 norg. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

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

enum { INTEGER=0, BIGINTEGER=1, STRING=2, LIST=3 };
enum {
    MEM_OUT_OF_IT=1,
    MEM_LOCKED,
    MEM_UNLOCKED,

    INTEGER_UNPACK_ON_INCORRECT_TYPE=10,
    BINDING_NODE_SIZE_REMAINING_NONZERO,
    RECURSION_DEPTH_EXCEEDED,
    LIST_FIRST_DIDNT_GET_LIST,

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

MEM *mem_malloc(uint16_t size) {
    MEM *mem = malloc(size);
    mem->start = mem + sizeof(MEM);
    mem->end = ((void*)mem) + size;

    mem->mark = (void**)(mem->end - sizeof(void*));
    mem->bottom = mem->end - sizeof(void*);
    *mem->mark = mem->bottom;
    mem->locked = FALSE;

    return mem;
}
void mem_unlock(MEM *m, void *new_bottom) {
    lassert(m->locked, MEM_UNLOCKED);
    lassert(m->start <= new_bottom, MEM_OUT_OF_IT);
    m->locked = FALSE;
    m->bottom = new_bottom;
}
void *mem_alloc(MEM *m, uint16_t size) {
    lassert(!m->locked, MEM_LOCKED);
    m->bottom -= size;
    lassert(m->start <= m->bottom, MEM_OUT_OF_IT);
    return m->bottom;
}
void mem_mark(MEM *m) {
    lassert(!m->locked, MEM_LOCKED);
    m->bottom -= sizeof(void*);
    *((void**)m->bottom) = m->mark;
    m->mark = &m->bottom;
}
void mem_free(MEM *m) {
    lassert(!m->locked, MEM_LOCKED);
    m->bottom = m->mark + sizeof(void*);
    m->mark = *m->mark;
}

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
            uint16_t list_modifiers;
        };
    };
} CELL;

int32_t cell_integer_unpack(CELL *cell) {
    dassert(cell->type == INTEGER, INTEGER_UNPACK_ON_INCORRECT_TYPE);
    int32_t res = (cell->length << 16) + cell->integer;
    if (cell->length & 0x20) {
        res = (-res) + 1;
    }
    return res;
}
CELL *cell_integer_pack(CELL *cell, int32_t val) {
    if (val < 0) {
        val = -val;
    }
    // integer should be 16bits, store extra bits in length
    cell->length = (val >> 16) & 0x3F;
    cell->integer = val;
    return cell;
}

CELL *add(MEM *m, CELL *list) {
    int32_t val = 0;
    for (uint8_t n=1; n<list->length; n++) {
        val += cell_integer_unpack(&list[n]);
    }
    CELL *c = mem_bottomalloc(m, sizeof(CELL));
    return cell_integer_pack(c, val);
}

CELL *subtract(MEM *m, CELL *list) {
    CELL *c = mem_bottomalloc(m, sizeof(CELL));
    int32_t val = 0;
    if (list->length >= 1) {
        val = cell_integer_unpack(&list[1]);
    }
    for (uint8_t n=2; n<list->length; n++) {
        val -= cell_integer_unpack(&list[n]);
    }
    return cell_integer_pack(c, val);
}

CELL *defn(MEM *m, CELL *list) {
    CELL *symbol = &list[1];
    uint8_t argcount = list[2].length;
    return list;
}

CELL *list_first(CELL *list) {
    lassert(list->type == LIST, LIST_FIRST_DIDNT_GET_LIST);
    return &list[1];
}

CELL *next_cell(CELL *cell) {
    uint8_t remaining[MAX_RECURSION];
    uint8_t depth = 0;
    if (cell->type != LIST) {
        return &cell[1];
    }
    remaining[depth] = cell->length;
    cell++;
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
        cell++;
    }
    return cell;
}


CELL *cond(MEM *m, CELL *list) {
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

CELL *iff(MEM * const mem, CELL * const list) {
    if (is_true(eval(mem, list[1]))) {
        return eval(mem, list[2]);
    } else if (list->length == 4) {
        return eval(mem, list[3]);
    }
}

CELL *let(MEM * const mem, CELL * const list) {
    return NULL;
}

CELL *append(MEM * const mem, CELL * const list) {

}

CELL *eq(MEM * const mem, CELL * const list) {
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

enum modifier {
    QUOTE, LAZY_EVAL,
};
typedef CELL* (*builtin_fn)(MEM *m, CELL *list);
typedef struct builtin_binding {
    builtin_fn fn;
    uint8_t modifier:2;
    uint8_t symbol_length:6;
    char *symbol;
} BUILTIN_BINDINGS[] = {
    { .fn = add, .modifier = 0, .symbol = "+" },
    { .fn = subtract, .modifier = 0, .symbol = "-"},
    { .fn = eq, .modifier = 0, .symbol = "="},
    { .fn = defn, .modifier = QUOTE, .symbol = "defn"},
    { .fn = cond, .modifier = LAZY_EVAL, .symbol = "cond"},
    { .fn = iff, .modifier = LAZY_EVAL, .symbol = "if"},
    { .fn = let, .modifier = QUOTE, .symbol = "let"},
    { .fn = append, .modifier = 0, .symbol = "append"},
};
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

BINDING_VLIST *bind_new(MEM *m, CELL *symbol, CELL *value) {
    BINDING_VLIST *bv = mem_bottomalloc(m, sizeof(BINDING_VLIST));
    bv->size = 0;
    bv->root = NULL;
    bv->marks = NULL;
    return bv;
};

void binding_push_mark(MEM *m, BINDING_VLIST *bv) {
    BINDING_MARK *mark = mem_bottomalloc(m, sizeof(BINDING_MARK));
    mark->size = bv->size;
    mark->parent = bv->marks;
    bv->marks = mark;
}

void binding_pop_mark(MEM *mem, BINDING_VLIST *bv) {
    bv->size = bv->marks->size;
    bv->marks = bv->marks->parent;
}

void bind(MEM *m, BINDING_VLIST *bv, CELL *symbol, CELL *value) {
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
        *node = mem_bottomalloc(m, SIZEOF_BINDING_VLIST_NODE(node_size));
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

    READER_STATE *state;
    READER_STATE *unused_states;

    BINDING_VLIST *bindings;
} READER;


static inline char reader_ungetc(READER *r, char c) {
    lassert(r->ungetbuff_i < sizeof(r->ungetbuff), READER_UNGETC_ERROR);
    r->ungetbuff[r->ungetbuff_i++] = c;
    return c;
}

static inline char reader_getc(READER *r) {
    char c;
    if (r->ungetbuff_i) {
        c = r->ungetbuff[--r->ungetbuff_i];
    } else {
        c = r->getc(r->getc_streamobj);
    }
    return c;
}

static inline char reader_peekc(READER *r) {
    if (r->ungetbuff_i) {
        return r->ungetbuff[r->ungetbuff_i-1];
    } else {
        char c = reader_getc(r);
        return reader_ungetc(r, c);
    }
}

static inline void reader_set_getc(
        READER *r,
        char (*getc)(void *),
        void *getc_streamobj) {
    r->getc = getc;
    r->getc_streamobj = getc_streamobj;
}

static inline void reader_set_putc(
        READER *r,
        char (*putc)(void *, char),
        void *putc_streamobj) {
    r->putc = putc;
    r->putc_streamobj = putc_streamobj;
}

static inline char reader_putc(READER *r, char c) {
    return r->putc(r->putc_streamobj, c);
}

static inline char is_whitespace(char c) {
    return c == '\n' || c == ' ' || c == '\t';
}

static inline char is_alpha(char c) {
    return (('A' <= c && c <= 'Z') ||
            ('a' <= c && c <= 'z') ||
            ('0' <= c && c <= '9'));
}

static inline char is_standard_char(char c) {
    return c >= '!' && c <= '~';
}

static inline char is_non_symbol(char c) {
    return c != '\n' && c != '\t' && c != ' ' && c != ')' && c != '(';
}

static inline char next_non_ws(READER *r) {
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

READER *reader_new(MEM *m) {
    READER *r = mem_bottomalloc(m, sizeof(READER));

    r->ungetbuff_i = 0;

    r->state = NULL;
    r->unused_states = NULL;
    return r;
}
enum {
    READING_STRING,
    READING_INTEGER,
    READING_ANYTHING,
};

void reader_push_cell(READER *r) {
    mem_topunlock(r->mem, r->mem->top);
    r->state->cell = mem_topalloc(r->mem, sizeof(CELL));
    mem_toplock(r->mem);
}

void reader_push_new_state(READER *reader) {
    READER_STATE *state;
    if (reader->unused_states) {
        state = reader->unused_states;
        reader->unused_states = state->parent;
    } else {
        state = mem_bottomalloc(reader->mem, sizeof(READER_STATE));
    }
    state->parent = reader->state;
    state->parse_state = READING_ANYTHING;
    reader->state = state;
}

void reader_pop_state(READER *r) {
    // pops READER_STATE off current list and puts it back in unused.
    READER_STATE *unused_state = r->state;
    r->state = r->state->parent;
    unused_state->parent = r->unused_states;
    r->unused_states = unused_state;
}

void reader_string_start(READER *r) {
    reader_push_new_state(r);
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

void reader_string_read(READER *r, char c) {
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

CELL *eval_string(READER *r, CELL *cell) {
    return cell;
}

CELL *reader_string_end(READER *r) {
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

void reader_integer_start(READER *r) {
    reader_push_new_state(r);
    reader_push_cell(r);
    r->state->cell->type = INTEGER;
    r->state->parse_state = READING_INTEGER;
    r->state->accumulator = 0;
    r->state->sign = 1;
}

void reader_integer_read(READER *r, char c) {
    if (c == '-' && r->state->accumulator == 0) {
        r->state->sign = -1;
    } else {
        uint32_t accumulator = r->state->accumulator;
        accumulator = (accumulator<<3) + (accumulator<<2);
        accumulator += '0' - c;
        r->state->accumulator = accumulator;
    }
}

void reader_integer_end(READER *r) {
    cell_integer_pack(r->state->cell, r->state->accumulator);
}


void reader_list_start(READER *r) {
    r->state->parse_state = READING_ANYTHING;

    mem_topunlock(r->mem, r->mem->top);
    r->state->cell = mem_topalloc(r->mem, sizeof(CELL));
    mem_toplock(r->mem);

    r->state->cell->type = LIST;
    r->state->cell->length = 0;
    reader_push_new_state(r);
}

void reader_list_read_element(READER *r, CELL *c) {
    reader_pop_state(r);
}

void reader_list_end(READER *r) {
    //
}

void reader_read(READER &r) {
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
