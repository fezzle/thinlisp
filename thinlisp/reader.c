#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "defines.h"
#include "reader.h"
#include "runtime.h"
#include "list.h"

ENVIRONMENT *environment_new(BISTACK *bs) {
  ENVIRONMENT *e = bistack_alloc(bs, sizeof(ENVIRONMENT));
  e->bs = bs;
  e->ungetbuff_i = 0;
  e->streamobj = NULL;

  e->total_symbols = 0;
  e->total_strlen = 0;
  e->total_astnodes = 0;
  return e;
}

READER *reader_new(ENVIRONMENT *e) {
  READER *r = bistack_alloc(e->bs, sizeof(READER));
  r->environment = e;
  r->in_comment = FALSE;
  return r;
}
 
AST_TYPE reader_next_cell(READER *reader) {
  if (next_non_ws(reader->environment) == -1) {
    return AST_NOTYPE;
  }

  AST_TYPE asttype = AST_NOTYPE;
  uint8_t chars_read = 0;
  while (TRUE) {
    char c = reader_getc(reader);
    if (c == -1) {
      // put back all the characters we read
      while (chars_read-->0) {
        reader_ungetc(reader, c);
      }
      return AST_NOTYPE;
    } else {
      chars_read++;
    }

    switch (c) {
    case '"':
      return (AST_TYPE){.type=AST_SYMBOL, .prefix=AST_DOUBLEQUOTE};
      
    case '\'':
      asttype.prefix = AST_SINGLEQUOTE;
      continue;
      
    case '`':
      asttype.prefix = AST_QUASIQUOTE;
      continue;

    case '(':
      return (AST_TYPE){.type=AST_LIST, .prefix=asttype.prefix};

    case ',':
      asttype.prefix = AST_COMMA;
      continue;
    
    case '@':
      if (asttype.prefix == AST_COMMA) {
        asttype.prefix = AST_COMMA_AT;
      } else {
        asttype.prefix = AST_AT;
      }
      continue;

    case '&':
      asttype.prefix = AST_AMPERSAND;
      continue;

    case ';':
      lassert(
        asttype.bitfield == AST_NOTYPE.bitfield, 
        READER_SYNTAX_ERROR, 
        "comment in expression");
      chars_read = 0;
      reader->in_comment = TRUE;
      if (reader_consume_comment(reader)) {
        continue;
      } else {
        return AST_NOTYPE;
      }

    case ')':
      return AST_TERMINATOR;
        
    default:
      if (((c & 0b110000) && (c >= '0' && c <= '9')) || (c == '-') || (c == '+')) {
        reader_ungetc(reader, c);
        return (AST_TYPE){.type=AST_INTEGER, .prefix=AST_NONE, .terminator=FALSE};

      } else if ((c >= '<') && (c <= 'z')) {
        reader_ungetc(reader, c);
        return (AST_TYPE){.type=AST_SYMBOL, .prefix=asttype.prefix, .terminator=FALSE};

      } else {
        lerror(READER_SYNTAX_ERROR, "unhandled: %c(%hhx)\n", c, c);
      }
    }
  }
  lerror(READER_SYNTAX_ERROR, "runtime error");
}

bool reader_integer_context(READER *reader, READER_CONTEXT *reader_context) {
  lassert(reader_context->asttype.type == AST_INTEGER, READER_STATE_ERROR);

  READER_INTEGER_CONTEXT *integer_context = reader_context->integer;
  CELLHEADER *header = integer_context->cellheader;
  uint16_t value = header->Integer.value;
  while (1) {
    char c = reader_getc(reader);
    if (c == -1) {
      return FALSE;

    } else if (value == 0 &&  (c == '-' || c == '+')) {
      header->Integer.sign = (c == '-' ? -1 : 1);

    } else if (c >= '0' && c <= '9') {
      value = value * 10 + (c - '0');

    } else {
      reader_ungetc(reader, c);
      header->Integer.value = value;
      return TRUE;
    }
  }
}

bool reader_symbol_context(READER *reader, READER_CONTEXT *reader_context) {
  lassert(reader_context->asttype.type == AST_SYMBOL, READER_STATE_ERROR);

  READER_SYMBOL_CONTEXT *symbol_context = reader_context->symbol;
  CELLHEADER *header = symbol_context->cellheader;
  char is_double_quote = reader_context->asttype.prefix == AST_DOUBLEQUOTE;

  while (1) {
    char c = reader_getc(reader);

    if (header->Symbol.length >= 64) {
      // 64 max chars in symbol/string
      continue;
    }
    if (c == -1) {
      return FALSE;
    
    } else if (c == '\\' && !symbol_context->is_escaped) {
      symbol_context->is_escaped = 1;
    
    } else if (symbol_context->is_escaped) {
      ((char*)(&header[1]))[header->Symbol.length++] = c;
    
    } else if ((c == '"' && is_double_quote) || is_whitespace(c)) {
      header->Symbol.hash = hashstr_8((char*)(&header[1]), header->Symbol.length);
      // we've been writing ahead in the bistack buffer, now must allocate everything written to
      bistack_allocf(reader->environment->bs, header->Symbol.length);
      return TRUE;

    } else {
      ((char*)(&header[1]))[header->Symbol.length++] = c;
    }
  }
}


bool reader_consume_comment(READER *reader) {
  // reads comment characters until newline
  while (reader->in_comment) {
    char c = reader_getc(reader);
    if (c == -1) {
      return FALSE;
    } else if (c == '\n') {
      reader->in_comment = FALSE;
    }
  }
  return FALSE;
}


READER_CONTEXT *new_reader_context(AST_TYPE asttype, BISTACK *bs) {
  READER_CONTEXT *rc = bistack_alloc(bs, sizeof(READER_CONTEXT));

  rc->asttype = asttype;

  switch (asttype.type) {
  case AST_LIST:
    rc->list = bistack_alloc(bs, sizeof(READER_LIST_CONTEXT));
    rc->list->cellheader = bistack_allocf(bs, sizeof(CELLHEADER));
    rc->list->cellheader->List.type = asttype.type;
    rc->list->cellheader->List.prefix = asttype.prefix;
    rc->list->cellheader->List.length = 0;
    rc->list->cell = NULL;
    break;
  case AST_SYMBOL:
    rc->symbol = bistack_alloc(bs, sizeof(READER_SYMBOL_CONTEXT));
    rc->symbol->cellheader = bistack_allocf(bs, sizeof(CELLHEADER));
    rc->symbol->is_escaped = 0;
    rc->symbol->cellheader->Symbol.type = asttype.type;
    rc->symbol->cellheader->Symbol.length = 0;
    break;
  case AST_INTEGER:
    rc->integer = bistack_alloc(bs, sizeof(READER_INTEGER_CONTEXT));
    rc->integer->cellheader = bistack_allocf(bs, sizeof(CELLHEADER));
    rc->integer->cellheader->Integer.type = asttype.type;
    rc->integer->cellheader->Integer.sign = 1;
    rc->integer->cellheader->Integer.value = 0;
    break;
  default:
    lerror(READER_SYNTAX_ERROR, "unknown type");
  }

  return rc;
}


bool reader_read(READER *reader) {
  BISTACK *bs = reader->environment->bs;
  
  // mark the stack to rewind when return
  bistack_mark(bs);
  LIST *reader_context_stack = list_new(bs);

  READER_CONTEXT *reader_context = reader->cell;

  while (1) {
    // parent_reader_context is the parent of the current reader_context, 
    // should always be type list
    READER_CONTEXT *parent_reader_context = list_first(reader_context_stack);
    lassert(
      parent_reader_context == NULL || 
      parent_reader_context->asttype.type == AST_LIST, 
      READER_STATE_ERROR);

    if (reader_context == NULL) {
      // find next cell
      AST_TYPE asttype = reader_next_cell(reader);
      if (asttype.bitfield == AST_NOTYPE.bitfield) {
        // EOF?, return TRUE iff no parent context (implying we must be EOF in root context)
        return parent_reader_context == NULL;
        
      } else if (asttype.terminator) {
        // list terminator
        if (parent_reader_context) {
          reader_context = parent_reader_context;
          list_shift(reader_context_stack);
          parent_reader_context = list_first(reader_context_stack);
        } else {
          return 1;
        }
      } else {
        // new cell
        reader_context = new_reader_context(asttype, bs);
        
        if (reader->cell == NULL) {
          // reader just beginning set root reader context to this
          reader->cell = reader_context;
        } else if (parent_reader_context != NULL) {
          // set parent_reader_context to be currently reading reader_context    
          parent_reader_context->list->cell = reader_context;
        } else {
          lerror(READER_STATE_ERROR, "shouldn't reach");
        }
      }
    }
    
    if (reader_context->asttype.type == AST_SYMBOL) {
      if (!reader_symbol_context(reader, reader_context)) {
        return 0;
      }
      if (parent_reader_context) {
        parent_reader_context->list->cellheader->List.length++;
      }
      reader_context = NULL;
      continue;
    }
    
    if (reader_context->asttype.type == AST_INTEGER) {
      if (!reader_integer_context(reader, reader_context)) {
        return 0;
      }
      if (parent_reader_context) {
        parent_reader_context->list->cellheader->List.length++;
      }
      reader_context = NULL;
      continue;
    }
    
    if (reader_context->asttype.type == AST_LIST) {
      list_unshift(reader_context_stack, bs, reader_context);
      reader_context = NULL;
      continue;
    }
  }
}



#ifdef READER_TEST
#include <stdio.h>
#include <string.h>
#include "tests/minunit.h"

int tests_run = 0;

char mygetc(void *streamobj) {
  int c = fgetc((FILE*)streamobj);
  if (c == EOF) {
    return -1;
  } else {
    return (char)c;
  }
}

static char * test_reader_sample() {
   BISTACK *bs = bistack_new(8096);
   bistack_pushdir(bs, BS_BACKWARD);
   ENVIRONMENT *environment = environment_new(bs);
   READER *reader = reader_new(environment);
   FILE *streamobj = fopen("tests/sample.lisp", "rb");
   reader_setio(reader, mygetc, streamobj);
   bool res = reader_read(reader);
   mu_assert("reader should complete", res);
   mu_assert("reader cell should not be null", reader->cell != NULL);

   mu_assert("read type should be list", reader->cell->asttype.type == AST_LIST);
   return 0;
}


static char *all_tests() {
  mu_run_test(test_reader_sample);
  return 0;
}

int main(int argc, char **argv) {
     char *result = all_tests();
     if (result != 0) {
         printf("%s\n", result);
     }
     else {
         printf("ALL TESTS PASSED\n");
     }
     printf("Tests run: %d\n", tests_run);
 
     return result != 0;
}

#endif
