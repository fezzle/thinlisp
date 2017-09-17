#ifndef READER_H
#define READER_H

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "defines.h"
#include "utils.h"
#include "list.h"
#include "bistack.h"
#include "runtime.h"
#include "thinlisp.h"

typedef struct reader READER;
typedef struct reader_context READER_CONTEXT;

typedef struct reader_symbolcontext {
    char is_escaped;
} READER_SYMBOL_CONTEXT;

typedef struct reader_integercontext {
} READER_INTEGER_CONTEXT;

typedef struct reader_listcontext {
    READER_CONTEXT *reader_context;
} READER_LIST_CONTEXT;

typedef struct reader_context {
    AST_TYPE asttype;
    CELLHEADER *cellheader;
    void *rewindmark;

    // the valid type below is identified by asttype.type
    union {
        READER_SYMBOL_CONTEXT *symbol;
        READER_INTEGER_CONTEXT *integer;
        READER_LIST_CONTEXT *list;
    };
} READER_CONTEXT;



typedef struct reader {
    ENVIRONMENT *environment;

    void *getc_streamobj;
    char (*getc)(void *getc_streamobj);

    void *putc_streamobj;
    char (*putc)(void *putc_streamobj, char c);

    char ungetbuff[4];

    uint8_t ungetbuff_i:4;
    uint8_t in_comment:1;

    READER_CONTEXT *reader_context;
    void *put_missing_context;
    void *pprint_context;
} READER;

ENVIRONMENT *environment_new(BISTACK *bs);
READER *reader_new(ENVIRONMENT *e);
READER *reader_init(READER *reader);
char reader_consume_comment(READER *reader);
char reader_read(READER *reader);
char reader_pprint(READER *reader);
bool reader_put_missing(READER *reader);
READER_CONTEXT *new_reader_context(AST_TYPE asttype, BISTACK *bs);

static inline char reader_getc(READER *r) {
    char c;
    if (r->ungetbuff_i) {
        c = r->ungetbuff[--r->ungetbuff_i];
    } else {
        c = r->getc(r->getc_streamobj);
    }
    return c;
}

static inline char reader_ungetc(READER *r, char c) {
    lassert(r->ungetbuff_i < sizeof(r->ungetbuff), READER_STATE_ERROR);
    r->ungetbuff[r->ungetbuff_i++] = c;
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

#endif
