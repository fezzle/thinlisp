#ifndef READER_H
#define READER_H

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "defines.h"
#include "utils.h"
#include "bistack.h"
#include "runtime.h"
#include "thinlisp.h"
#include "cell.h"

enum {
    READER_READING_NEXT_SYMBOL=1,
    READER_READING_SYMBOL, 
    READER_READING_SYMBOL_DOUBLEQUOTED,
    READER_READING_SYMBOL_DOUBLEQUOTED_ESCAPED,
    READER_READING_SYMBOL_DONE,
    READER_READING_INTEGER_DONE,
    READER_READING_PREFIX,
    READER_READING_LIST,

    READER_READING_COMMENT,
    
    READER_READING_NEGATIVE_INTEGER,
    READER_READING_INTEGER,
    READER_READING_DONE,
};


enum {
    READER_IN_QUASIQUOTE=1,

};

typedef struct reader_context {
    CELL *cell;
    void *rewindmark;
    struct reader_context *parent;
    uint8_t state;
    bistack_lock_t stack_lock;
} READER_CONTEXT;


typedef struct reader {
    ENVIRONMENT *environment;

    void *getc_streamobj;
    char (*getc)(void *getc_streamobj);
    address_t (*getc_address)(void *getc_streamobj);

    void *putc_streamobj;
    char (*putc)(void *putc_streamobj, char c);

    char ungetbuff[4];

    uint8_t ungetbuff_i : 2;
    uint8_t state;

    READER_CONTEXT *reader_context;
    void *put_missing_context;
    void *pprint_context;
} READER;


ENVIRONMENT *environment_new(BISTACK *bs);
READER *reader_new(ENVIRONMENT *e);
READER *reader_init(READER *reader);
bool_t reader_consume_comment(READER *reader);
bool_t reader_read(READER *reader);
bool_t reader_pprint(READER *reader);
bool_t reader_put_missing(READER *reader);

READER_CONTEXT *new_reader_context(BISTACK *bs, READER_CONTEXT *parent);

static inline char reader_getc(READER *r) {
    char c;
    if (r->ungetbuff_i) {
        c = r->ungetbuff[--r->ungetbuff_i];
    } else {
        c = r->getc(r->getc_streamobj);
    }
    return c;
}

static inline address_t reader_getc_address(READER *r) {
    return r->getc_address(r->getc_streamobj);
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
        address_t (*getc_address)(void *),
        void *getc_streamobj) {
    r->getc = getc;
    r->getc_address = getc_address;
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
