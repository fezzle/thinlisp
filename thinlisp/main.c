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

enum {
    MEM_OUT_OF_IT, MEM_TOP_LOCKED, MEM_TOP_UNLOCKED,
    MEM_BOTTOM_LOCKED, MEM_BOTTOM_UNLOCKED,
    READER_UNGETC_ERROR
};

typedef struct mem {
    void *start;
    void *end;
    
    void *top;
    void **top_mark;

    void *bottom;
    void **bottom_mark;

    uint8_t top_locked:1;
    uint8_t bottom_locked:1;
} MEM;

MEM *mem_malloc(uint16_t size) {
    MEM *mem = malloc(size);
    mem->start = mem + sizeof(MEM);
    mem->end = ((void*)mem) + size;

    // top and bottom are 1ptr away from each end to make room for mark pointers
    mem->top_mark = (void**)mem->start;
    mem->top = mem->start + sizeof(void*);
    *mem->top_mark = mem->top_mark;
    mem->top_locked = FALSE;

    mem->bottom_mark = (void**)(mem->end - sizeof(void*));
    mem->bottom = mem->end - sizeof(void*);
    *mem->bottom_mark = mem->bottom;
    mem->bottom_locked = FALSE;

    return mem;
}
void *mem_toplock(MEM *m) {
    lassert(!m->top_locked, MEM_TOP_LOCKED);
    m->top_locked = TRUE;
    return m->top;
}
void mem_topunlock(MEM *m, void *new_top) {
    lassert(m->top_locked, MEM_TOP_UNLOCKED);
    lassert(new_top <= m->bottom, MEM_OUT_OF_IT);
    m->top_locked = FALSE;
    m->top = new_top;
}
void mem_topmark(MEM *m) {
    lassert(!m->top_locked, MEM_TOP_LOCKED);
    *((void**)m->top) = m->top_mark;
    m->top_mark = &m->top;
    m->top += sizeof(void *);
}
void *mem_topalloc(MEM *m, uint16_t size) {
    lassert(!m->top_locked, MEM_TOP_LOCKED);
    void *res = m->top;
    m->top += size;
    lassert(m->top <= m->bottom, MEM_OUT_OF_IT);
    return res;
}
void mem_topfree(MEM *m) {
    lassert(!m->top_locked, MEM_TOP_LOCKED);
    m->top = &m->top_mark;
    m->top_mark = *m->top_mark;
}
void *mem_bottomlock(MEM *m) {
    lassert(!m->bottom_locked, MEM_BOTTOM_LOCKED);
    m->bottom_locked = TRUE;
    return m->bottom;
}
void mem_bottomunlock(MEM *m, void *new_bottom) {
    lassert(m->bottom_locked, MEM_BOTTOM_UNLOCKED);
    lassert(m->top <= new_bottom, MEM_OUT_OF_IT);
    m->bottom_locked = FALSE;
    m->bottom = new_bottom;
}
void *mem_bottomalloc(MEM *m, uint16_t size) {
    lassert(!m->bottom_locked, MEM_BOTTOM_LOCKED);
    m->bottom -= size;
    lassert(m->top <= m->bottom, MEM_OUT_OF_IT);
    return m->bottom;
}
void mem_bottommark(MEM *m) {
    lassert(!m->bottom_locked, MEM_BOTTOM_LOCKED);
    m->bottom -= sizeof(void*);
    *((void**)m->bottom) = m->bottom_mark;
    m->bottom_mark = &m->bottom;
}
void mem_bottomfree(MEM *m) {
    lassert(!m->bottom_locked, MEM_BOTTOM_LOCKED);
    m->bottom = m->bottom_mark + sizeof(void*);
    m->bottom_mark = *m->bottom_mark;
}

typedef struct environment ENVIRONMENT;

typedef struct reader {
    void *getc_streamobj;
    char (*getc)(void *getc_streamobj);

    void *putc_streamobj;
    char (*putc)(void *putc_streamobj, char c);

    char ungetbuff[4];

    uint8_t ungetbuff_i : 2;
    uint8_t state;

    char *intbuff;
    uint8_t pos;

    ENVIRONMENT *e;
} READER;

typedef struct environment {
    MEM *mem;
    READER *reader;
} ENVIRONMENT;

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
        //address_t (*getc_address)(void *),
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


ENVIRONMENT *environment_new(MEM *m) {
    ENVIRONMENT *e = (ENVIRONMENT*)mem_topalloc(m, sizeof(ENVIRONMENT));
    e->mem = m;
    return e;
}
enum {
    STRING_OR_LIST,
    STRING_DISPLAY, 
    STRING_RAW_DECIMAL,     
    STRING_TOKEN,
};

void start_string_display(READER *r) {
    r->state = STRING_DISPLAY;
}
void start_string_raw_decimal(READER *r) {
    r->state = STRING_RAW_DECIMAL;
    r->pos = 0;
}
void read_string_raw_decimal(READER *r, char c) {
    r->intbuff[r->pos++] = c;
}
void start_string_token(READER *r) {
    r->state = STRING_TOKEN;
    MEM *m = r->environment->mem;
    void *mem_toplock(m);
}
void read_string_token(READER *r, char c) {
    // todo
}


READER *environment_read(ENVIRONMENT *e) {
    READER *r = e->reader;

    while (TRUE) {
        char c = reader_getc(r);
        switch (r->state) {

        case STRING_OR_LIST:
            if (c == '[') {
                // <sexpr> -> <string> -> <display>
                start_string_display(r);
            } else if (c >= '0' && c <= '9') {
                // <sexpr> -> <string> -> <simple-string> -> <raw> -> <decimal>
                // <sexpr> -> <string> -> <simple-string> -> <token>
                // <sexpr> -> <string> -> <simple-string> -> <base-64> -> <decimal>
                start_string_raw_decimal(r);
                read_string_raw_decimal(r, c);
            } else if (is_alpha_decimal_simplepunc(c)) {
                start_string_token(r);
                read_string_raw_decimal
            }
        }


        }
    }
}

READER *reader_init(READER *r) {
    r->ungetbuff_i = 0;
    r->state = 0;
    return r;
}

READER *reader_new(ENVIRONMENT *e) {
    READER *r = (READER*)bistack_heapalloc(e->bs, sizeof(READER));
    return reader_init(r);
}
bool_t reader_consume_comment(READER *reader);
bool_t reader_read(READER *reader);
bool_t reader_pprint(READER *reader);
bool_t reader_put_missing(READER *reader);



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
    BISTACK *bs = bistack_new(1<<18);
    bistack_pushdir(bs, BS_BACKWARD);
    ENVIRONMENT *environment = environment_new(bs);
    READER *reader = reader_new(environment);

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
        printf("\n *** %s\n", thrown_error_to_string(exctype));
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

int mygetc(void *s) {
    return getchar();
}
char myputc(void *streamobj, char c) {
    return putchar(c);
}

int main(int argc, char **argv) {
    BISTACK *bs = (BISTACK*)malloc(32000);
    bistack_init(bs, 32000);
    ENVIRONMENT *e = environment_new(bs);
    READER *r = reader_new(e);

    reader_set_getc(r, mygetc, NULL);
    reader_set_putc(r, myputc, NULL);
    reader_read(r);
}
#endif
