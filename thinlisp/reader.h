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


typedef struct reader READER;
typedef struct reader_context READER_CONTEXT;

typedef struct reader_symbolcontext {
  CELLHEADER *cellheader;
  char is_escaped;
} READER_SYMBOL_CONTEXT;

typedef struct reader_integercontext {
  CELLHEADER *cellheader;
} READER_INTEGER_CONTEXT;

typedef struct reader_listcontext {
  CELLHEADER *cellheader;
  READER_CONTEXT *cell;
} READER_LIST_CONTEXT;

typedef struct reader_context {
  struct {
    AST_TYPE asttype;
    union {
      READER_SYMBOL_CONTEXT *symbol;
      READER_INTEGER_CONTEXT *integer;
      READER_LIST_CONTEXT *list;
    };
  };
} READER_CONTEXT;


typedef struct environment { 
  // total strlen of the non-global symbols
  uint16_t total_symbols;
  uint16_t total_strlen;
  uint16_t total_astnodes;
  
  BISTACK *bs;

  void *streamobj;
  char (*getc)(void *streamobj);

  char ungetbuff[4];
  uint8_t ungetbuff_i;
} ENVIRONMENT;

typedef struct reader {
  READER *parent;
  ENVIRONMENT *environment;

  uint8_t in_comment;

  READER_CONTEXT *cell;

} READER;

ENVIRONMENT *environment_new(BISTACK *bs);
READER *reader_new(ENVIRONMENT *e);
char reader_consume_comment(READER *reader);


static inline char environment_getc(ENVIRONMENT *e) {
  char c;
  if (e->ungetbuff_i) {
    c = e->ungetbuff[--e->ungetbuff_i];
  } else {
    c = e->getc(e->streamobj);
  }
  return c;
}

static inline char reader_getc(READER *r) {
  return environment_getc(r->environment);
}

static inline char environment_ungetc(ENVIRONMENT *e, char c) {
  assert(e->ungetbuff_i < sizeof(e->ungetbuff));
  e->ungetbuff[e->ungetbuff_i++] = c;
  return c;
}
static inline char reader_ungetc(READER *r, char c) {
  return environment_ungetc(r->environment, c);
}

static inline char environment_peekc(ENVIRONMENT *e) {
  if (e->ungetbuff_i) {
    return e->ungetbuff[e->ungetbuff_i-1];
  } else {
    char c = environment_getc(e);
    return environment_ungetc(e, c);
  }
}
static inline char reader_peekc(READER *r) {
  return environment_peekc(r->environment);
}

static inline void environment_setio(ENVIRONMENT *e, char (*getc)(void *ctx), void *streamobj) {
  e->getc = getc;
  e->streamobj = streamobj;
}

static inline void reader_setio(READER *r, char (*getc)(void *ctx), void *streamobj) {
  environment_setio(r->environment, getc, streamobj);
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

static inline char next_non_ws(ENVIRONMENT *e) {
  /*
   * reads characters until a non-whitespace character is found.
   */
  char c = ' ';
  while (c != -1 && is_whitespace(c)) {
    c = environment_getc(e);
  }
  if (c != -1) {
    // rewind 1
    environment_ungetc(e, c);
  }
  return c;
}

#endif
