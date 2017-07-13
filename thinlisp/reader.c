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
  e->total_symbols = 0;
  e->total_strlen = 0;
  e->total_astnodes = 0;
  return e;
}

READER *reader_new(ENVIRONMENT *e) {
  READER *r = bistack_alloc(e->bs, sizeof(READER));
  r->environment = e;
  r->in_comment = FALSE;
  r->reader_context = NULL;
  r->ungetbuff_i = 0;
  r->putc_streamobj = NULL;
  r->putc = NULL;
  r->getc_streamobj = NULL;
  r->getc = NULL;

  return r;
}

AST_TYPE reader_next_cell(READER *reader) {
  if (next_non_ws(reader) == -1) {
    return AST_NOTYPE;
  }

  AST_TYPE asttype = AST_NOTYPE;
  char chars_read_buffer[4];
  char chars_read = 0;
  while (TRUE) {
    char c = reader_getc(reader);
    lassert(chars_read < sizeof(chars_read_buffer), READER_STATE_ERROR);
    chars_read_buffer[chars_read++] = c;

    if (c == -1) {
      // put back all the characters we read
      while (chars_read-- > 0) {
        reader_ungetc(reader, chars_read_buffer[chars_read]);
      }
      return AST_NOTYPE;
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

    case '+':
      asttype.prefix = AST_PLUS;
      continue;

    case '-':
      asttype.prefix = AST_MINUS;
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

    case ' ':
      // space encountered while reading start of new cell.  maybe a symbol?
      while (chars_read-- > 0) {
        reader_ungetc(reader, chars_read_buffer[chars_read]);
      }
      return (AST_TYPE){.type=AST_SYMBOL, .prefix=AST_NONE, .terminator=FALSE};

    default:
      if (((c & 0b110000) && (c >= '0' && c <= '9'))) {
        reader_ungetc(reader, c);
        return (AST_TYPE){.type=AST_INTEGER, .prefix=asttype.prefix, .terminator=FALSE};

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
      header->Integer.sign = (c == '-' ? 0 : 1);

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

    } else if (c == '"' && is_double_quote) {
      break;

    } else if (!is_double_quote &&
        (is_whitespace(c) || (c == ')') || (c == ';'))) {
      // put back character that ends non-double-quoted symbol
      reader_ungetc(reader, c);
      break;

    } else {
      ((char*)(&header[1]))[header->Symbol.length++] = c;
    }
  }
  header->Symbol.hash = hashstr_8((char*)(&header[1]), header->Symbol.length);

  bistack_allocf(reader->environment->bs, header->Symbol.length);
  return TRUE;
}

void putc_tens(READER *r, uint16_t value, uint16_t tens) {
  char c='0';
  for (; value > tens; value -= tens) {
    c++;
  }
  reader_putc(r, c);
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
  bistack_mark(bs);
  READER_CONTEXT *rc = bistack_alloc(bs, sizeof(READER_CONTEXT));

  rc->asttype = asttype;

  switch (asttype.type) {
  case AST_LIST:
    rc->list = bistack_alloc(bs, sizeof(READER_LIST_CONTEXT));
    rc->list->cellheader = bistack_allocf(bs, sizeof(CELLHEADER));
    rc->list->cellheader->List.type = asttype.type;
    rc->list->cellheader->List.prefix = asttype.prefix;
    rc->list->cellheader->List.length = 0;
    rc->list->reader_context = NULL;
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

void destroy_reader_context(BISTACK *bs, READER_CONTEXT *reader_context) {
  bistack_rewind(bs);
}


bool reader_read(READER *reader) {
  BISTACK *bs = reader->environment->bs;

  // mark the stack to rewind when return
  bistack_mark(bs);
  LIST *reader_context_stack = list_new(bs);

  // reader_contexts are hierachical through their ->list to provide
  // continuation like behaviour
  READER_CONTEXT *reader_context = reader->reader_context;
  while (reader_context != NULL && reader_context->asttype.type == AST_LIST) {
    list_unshift(reader_context_stack, bs, reader_context);
    reader_context = reader_context->list->reader_context;
  }

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
        destroy_reader_context(bs, NULL);
        reader_context = list_shift(reader_context_stack);
        if (reader_context == NULL) {
          // no parent context, this was the root.  reading completed.
          return 1;
        }
        parent_reader_context = list_first(reader_context_stack);
        if (parent_reader_context != NULL) {
          parent_reader_context->list->cellheader->List.length++;
        }
      } else {
        // new cell
        reader_context = new_reader_context(asttype, bs);

        if (reader->reader_context == NULL) {
          // reader just beginning set root reader context to this
          reader->reader_context = reader_context;
        } else if (parent_reader_context != NULL) {
          // set parent_reader_context to be currently reading reader_context
          parent_reader_context->list->reader_context = reader_context;
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
        parent_reader_context->list->reader_context = NULL;
      }
      destroy_reader_context(bs, reader_context);
      reader_context = NULL;
      continue;
    }

    if (reader_context->asttype.type == AST_INTEGER) {
      if (!reader_integer_context(reader, reader_context)) {
        return 0;
      }
      if (parent_reader_context) {
        parent_reader_context->list->cellheader->List.length++;
        parent_reader_context->list->reader_context = NULL;
      }
      destroy_reader_context(bs, reader_context);
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


void reader_pprint(READER *reader) {
  ENVIRONMENT *e = reader->environment;
  lassert(reader->putc != NULL, READER_STATE_ERROR);

  uint8_t indent = 0;

  LIST *cellstack = list_new(e->bs);

  typedef struct frame {
    CELLHEADER *cellheader;
    uint8_t subcellcounter;
  } FRAME;

  READER_CONTEXT *reader_context = reader->reader_context;
  CELLHEADER *cellheader = (
    reader_context->asttype.type == AST_LIST ? reader_context->list->cellheader :
    reader_context->asttype.type == AST_SYMBOL ? reader_context->symbol->cellheader :
    reader_context->integer->cellheader);
  while (cellheader) {
    FRAME *parentcellheader = list_first(cellstack);
    if (cellheader->Symbol.type == AST_SYMBOL) {
      for (int i=0; i<cellheader->Symbol.length; i++) {
        reader_putc(reader, ((char*)(&cellheader[1]))[i]);
      }
      reader_putc(reader, ' ');
      cellheader = ((void*)cellheader + sizeof(cellheader)) + cellheader->Symbol.length;
      if (parentcellheader) {
        parentcellheader->subcellcounter--;
      }
    }
    else if (cellheader->Integer.type == AST_INTEGER) {
      reader_putc(reader, cellheader->Integer.sign ? '+' : '-');
      uint16_t value = cellheader->Integer.value;
      putc_tens(reader, value, 10000);
      putc_tens(reader, value, 1000);
      putc_tens(reader, value, 100);
      putc_tens(reader, value, 10);
      putc_tens(reader, value, 1);
      reader_putc(reader, ' ');
      cellheader = (void*)cellheader + sizeof(cellheader);
      if (parentcellheader) {
        parentcellheader->subcellcounter--;
      }
    }
    else if (cellheader->List.type == AST_LIST) {
      for (uint8_t n=0; n<indent; n++) {
        reader_putc(reader, ' ');
      }
      reader_putstr(reader, AST_PREFIX_STR(cellheader->List.prefix));
      reader_putc(reader, '(');

      FRAME *newframe = bistack_alloc(e->bs, sizeof(FRAME));
      newframe->subcellcounter = cellheader->List.length;
      newframe->cellheader = cellheader;
      list_unshift(cellstack, e->bs, newframe);
      cellheader = (CELLHEADER*)&cellheader[1];

      if (parentcellheader) {
        parentcellheader->subcellcounter--;
      }
      parentcellheader = list_first(cellstack);
      indent++;
    }

    if (parentcellheader == NULL) {
      reader_putc(reader, '\n');
      if (cellheader->List.type != AST_LIST) {
        cellheader = NULL;
      }
    } else if (/*parentcellheader != null &&*/parentcellheader->subcellcounter == 0) {
      reader_putc(reader, ')');
      reader_putc(reader, '\n');
      cellheader = list_shift(cellstack);
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

char myputc(void *streamobj, char c) {
  static char lastchar = 0;
  if (lastchar == '\n') {
    printf("***   ");
  }
  printf("%c", c);
  lastchar = c;
  return c;
}

static char * test_reader_sample() {
   BISTACK *bs = bistack_new(1<<14);
   bistack_pushdir(bs, BS_BACKWARD);
   ENVIRONMENT *environment = environment_new(bs);
   READER *reader = reader_new(environment);
   FILE *streamobj = fopen("tests/sample.lisp", "rb");
   reader_set_getc(reader, mygetc, streamobj);
   bool res = reader_read(reader);
   mu_assert("reader should complete", res);
   mu_assert("reader reader_context should not be null", reader->reader_context != NULL);

   mu_assert("read type should be list", reader->reader_context->asttype.type == AST_LIST);
   return 0;
}



static char * test_reader_sample_pprint() {
   BISTACK *bs = bistack_new(1<<14);
   bistack_pushdir(bs, BS_BACKWARD);
   ENVIRONMENT *environment = environment_new(bs);
   READER *reader = reader_new(environment);
   FILE *streamobj = fopen("tests/sample.lisp", "rb");
   reader_set_getc(reader, mygetc, streamobj);
   reader_set_putc(reader, myputc, NULL);
   reader_read(reader);
   reader_pprint(reader);
   mu_assert("read type should be list", reader->reader_context->asttype.type == AST_LIST);
   return 0;
}


static char *all_tests() {
  mu_run_test(test_reader_sample);
  mu_run_test(test_reader_sample_pprint);
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
