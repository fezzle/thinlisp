#ifndef READER_H
#define READER_H

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "defines.h"
#include "utils.h"
#include "list.h"
#include "bistack.h"
#include "avl.h"

typedef struct reader_listcontext {
  AST_TYPE type;
  struct reader_listcontext *next;
  LIST *list;
} READER_LIST_CONTEXT;


typedef struct reader_atomcontext {
  MULTISTRING *string;
  AST_TYPE type;
  char escaped;
} READER_ATOM_CONTEXT;


typedef struct reader {
  BISTACK *bs;
  READER_LIST_CONTEXT *root_list;
  READER_LIST_CONTEXT *current_list;
  READER_ATOM_CONTEXT *current_atom;
  
  // Symbol String table (as opposed to bound-value table)
  AVL_NODE *obhash; // like oblist, but avl tree
  AVL_NODE *global_obhash;

  // total strlen of the non-global symbols
  uint16_t total_symbols;
  uint16_t total_strlen;
  uint16_t total_astnodes;
  
  void *streamobj;
  char (*getc)(void *streamobj);
  uint8_t in_comment;
  char ungetbuff[4];
  uint8_t ungetbuff_i;
} READER;

READER *reader_new(BISTACK *bs);


#include <stdio.h>
static inline char reader_getc(READER *r) {
  char c;
  if (r->ungetbuff_i) {
    c = r->ungetbuff[--r->ungetbuff_i];
  } else {
    c = r->getc(r->streamobj);
  }
  return c;
}

static inline char reader_ungetc(READER *r, char c) {
  assert(r->ungetbuff_i < sizeof(r->ungetbuff));
  r->ungetbuff[r->ungetbuff_i++] = c;
  return c;
}

static inline char reader_peekc(READER *r) {
  if (r->ungetbuff_i) {
    return r->ungetbuff[r->ungetbuff_i-1];
  } else {
    char c = r->getc(r->streamobj);
    return reader_ungetc(r, c);
  }
}

AST_NODE *read_atom(READER *reader);
AST_NODE *reader_continue(READER *reader);

void reader_pprint(READER *reader, AST_NODE *node, uint8_t indentspaces);

static inline void reader_setio(READER *reader, char (*getc)(void *ctx)) {
  reader->getc = getc;
}

static inline int is_whitespace(char c) {
  return c == '\n' || c == ' ' || c == '\t';
}

static inline int is_standard_char(char c) {
  return c >= '!' && c <= '~';
}

static inline int is_non_symbol(char c) {
  return c != ' ' && c != ')' && c != '(';
}

static inline char next_non_ws(READER *reader) {
  /*
   * reads characters until a non-whitespace character is found.
   */
  char c = ' ';
  while (c != -1 && is_whitespace(c)) {
    c = reader_getc(reader);
  }
  if (c != -1) {
    // rewind 1
    reader_ungetc(reader, c);
  }
  return c;
}

#endif

