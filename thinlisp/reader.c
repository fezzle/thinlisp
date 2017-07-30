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
  r->is_completed = FALSE;
  r->reader_context = NULL;
  r->ungetbuff_i = 0;
  r->putc_streamobj = NULL;
  r->putc = NULL;
  r->getc_streamobj = NULL;
  r->getc = NULL;

  return r;
}


static const AST_TYPE AST_NOTYPE = ((AST_TYPE) {
  .type = AST_NONE,
  .prefix = AST_NONE,
  .terminator = AST_NONE,
  });

static const AST_TYPE AST_TERMINATOR = ((AST_TYPE) {
  .type = AST_LIST,
  .prefix = AST_NONE,
  .terminator = TRUE,
  });

AST_TYPE reader_next_cell(READER *reader) {
  if (next_non_ws(reader) == -1) {
    return AST_NOTYPE;
  }

  AST_TYPE asttype = AST_NOTYPE;
  char chars_read_buffer[4];
  char chars_read = 0;
  while (TRUE) {
    char c = reader_getc(reader);
    if (c == -1) {
      // put back all the characters we read
      while (chars_read-- > 0) {
        reader_ungetc(reader, chars_read_buffer[chars_read]);
      }
      return AST_NOTYPE;
    }
    lassert(chars_read < sizeof(chars_read_buffer), READER_STATE_ERROR);
    chars_read_buffer[chars_read++] = c;

    switch (c) {
    case '"':
      // put back double quote so quotes are part of the symbol
      reader_ungetc(reader, c);
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
        return (AST_TYPE){
          .type=AST_INTEGER,
          .prefix=asttype.prefix,
          .terminator=FALSE};

      } else if ((c >= '<') && (c <= 'z')) {
        reader_ungetc(reader, c);
        return (AST_TYPE){
          .type=AST_SYMBOL,
          .prefix=asttype.prefix,
          .terminator=FALSE};

      } else {
        lerror(READER_SYNTAX_ERROR, "unhandled: %c(%hhx)\n", c, c);
      }
    }
  }
  lerror(READER_SYNTAX_ERROR, "runtime error");
}

bool reader_integer_context(READER *reader, READER_CONTEXT *reader_context) {
  lassert(reader_context->asttype.type == AST_INTEGER, READER_STATE_ERROR);

  CELLHEADER *header = reader_context->cellheader;
  uint16_t value = header->Integer.value;
  while (1) {
    char c = reader_getc(reader);
    if (c == -1) {
      return FALSE;

    } else if (value == 0 &&  (c == '-' || c == '+')) {
      header->Integer.sign = (c == '-' ? 0 : 1);

    } else if (c >= '0' && c <= '9') {
      value = value * 10 + (c - '0');
      header->Integer.value = value;

    } else {
      reader_ungetc(reader, c);
      return TRUE;
    }
  }
}

bool reader_symbol_context(READER *reader, READER_CONTEXT *reader_context) {
  lassert(reader_context->asttype.type == AST_SYMBOL, READER_STATE_ERROR);

  READER_SYMBOL_CONTEXT *symbol_context = reader_context->symbol;
  CELLHEADER *header = reader_context->cellheader;
  char is_double_quoted = reader_context->asttype.prefix == AST_DOUBLEQUOTE;

  char finished = FALSE;
  while (! finished) {
    char c = reader_getc(reader);

    if (c == -1) {
      return FALSE;

    } else if (c == '\\' && !symbol_context->is_escaped) {
      // accept character and set is_escaped
      symbol_context->is_escaped = 1;

    } else if (symbol_context->is_escaped) {
      // accept character

    } else if (c == '"') {
      // indicates end iff symbol is double quoted and past first character
      finished = is_double_quoted && header->Symbol.length > 0;

    } else if (!is_double_quoted &&
        (is_whitespace(c) || (c == ')') || (c == ';'))) {
      // put back character that ends non-double-quoted symbol
      reader_ungetc(reader, c);
      break;
    }

    if (header->Symbol.length < ((1 << 6) - 1)) {
      ((char*)(&header[1]))[header->Symbol.length++] = c;
    }
  }

  header->Symbol.hash = hashstr_8((char*)(&header[1]), header->Symbol.length);

  bistack_allocf(reader->environment->bs, header->Symbol.length);
  return TRUE;
}

/**
 * @return TRUE iff a non-zero digit was printed
 */
char putc_tens(READER *r, uint16_t value, uint16_t tens, char print_zero) {
  char c = '0';
  for (; value >= tens; value -= tens) {
    c++;
  }
  if (print_zero || c > '0') {
    reader_putc(r, c);
  }
  return c > '0';
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
  /**
   * Marks and allocates a READER_CONTEXT on the default(BACKWARD) stack of BS.
   * Allocates a new CELLHEADER on the FORWARD stack of BS.
   */
  void *rewindmark = bistack_mark(bs);
  READER_CONTEXT *rc = bistack_alloc(bs, sizeof(READER_CONTEXT));

  rc->asttype = asttype;
  rc->rewindmark = rewindmark;

  switch (asttype.type) {
  case AST_LIST:
    rc->list = bistack_alloc(bs, sizeof(READER_LIST_CONTEXT));
    rc->list->reader_context = NULL;
    rc->cellheader = bistack_allocf(bs, sizeof(CELLHEADER));
    rc->cellheader->List.type = asttype.type;
    rc->cellheader->List.prefix = asttype.prefix;
    rc->cellheader->List.length = 0;
    break;
  case AST_SYMBOL:
    rc->symbol = bistack_alloc(bs, sizeof(READER_SYMBOL_CONTEXT));
    rc->symbol->is_escaped = 0;
    rc->cellheader = bistack_allocf(bs, sizeof(CELLHEADER));
    rc->cellheader->Symbol.type = asttype.type;
    rc->cellheader->Symbol.length = 0;
    break;
  case AST_INTEGER:
    rc->integer = bistack_alloc(bs, sizeof(READER_INTEGER_CONTEXT));
    rc->cellheader = bistack_allocf(bs, sizeof(CELLHEADER));
    rc->cellheader->Integer.type = asttype.type;
    rc->cellheader->Integer.sign = 1;
    rc->cellheader->Integer.value = 0;
    break;
  default:
    lerror(READER_SYNTAX_ERROR, "unknown type");
  }

  return rc;
}

/**
 * Destroys a reader context and asserts the rewind mark matches its mark
 */
void destroy_reader_context(BISTACK *bs, READER_CONTEXT *reader_context) {
  void *rewind_mark = bistack_rewind(bs);
  lassert(rewind_mark == reader_context->rewindmark, READER_STATE_ERROR);
}


char reader_read(READER *reader) {
  BISTACK *bs = reader->environment->bs;

  while (1) {
    // reader_contexts are hierachical through their ->list->reader_context
    READER_CONTEXT *reader_context = reader->reader_context;

    // iterate through heirarchy of reader_contexts to find bottom and parent
    READER_CONTEXT *parent_reader_context = NULL;
    READER_CONTEXT *parent_parent_reader_context = NULL;
    while (reader_context && reader_context->asttype.type == AST_LIST) {
      parent_parent_reader_context = parent_reader_context;
      parent_reader_context = reader_context;
      reader_context = reader_context->list->reader_context;
    }

    lassert(
      parent_reader_context == NULL ||
      parent_reader_context->asttype.type == AST_LIST,
      READER_STATE_ERROR);

    if (reader_context == NULL) {
      // find next cell
      AST_TYPE asttype = reader_next_cell(reader);
      if (asttype.bitfield == AST_NOTYPE.bitfield) {
        // this is perplexing.  hit EOF while reading first cell or reading
        //  next cell in list
        // EOF?, return TRUE iff no parent context
        //  (implying we must be EOF in root context)
        //lassert(parent_reader_context == NULL, READER_STATE_ERROR);
        return 0;

      } else if (asttype.terminator) {
        // asttype has terminator flag indicated end of list
        lassert(asttype.type == AST_LIST, READER_STATE_ERROR);
        lassert(parent_reader_context != NULL, READER_STATE_ERROR);

        // "destroy" the parent_reader_context here, but still use struct below
        destroy_reader_context(bs, parent_reader_context);
        if (parent_reader_context == reader->reader_context) {
          // root reader has completed, terminate
          reader->is_completed = TRUE;
          reader->cell = (CELL*)parent_reader_context->cellheader;
          reader->reader_context = NULL;
          return 1;

        } else {
          // parent_reader_context list is ending, update parent_parent_reader
          lassert(parent_parent_reader_context != NULL, READER_STATE_ERROR);
          parent_parent_reader_context->cellheader->List.length++;
          parent_parent_reader_context->list->reader_context = NULL;
          continue;
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

    uint8_t asttype_type = reader_context->asttype.type;

    if (asttype_type == AST_INTEGER || asttype_type == AST_SYMBOL) {
      if (asttype_type == AST_SYMBOL) {
        if (!reader_symbol_context(reader, reader_context)) {
          return 0;
        }
      } else if (asttype_type == AST_INTEGER) {
        if (!reader_integer_context(reader, reader_context)) {
          return 0;
        }
      }
      destroy_reader_context(bs, reader_context);
      reader_context = NULL;
      if (parent_reader_context) {
        parent_reader_context->cellheader->List.length++;
        parent_reader_context->list->reader_context = NULL;
        continue;
      } else {
        // root reader_context terminating symbol read
        lassert(reader_context == reader->reader_context, READER_STATE_ERROR);
        reader->is_completed = TRUE;
        reader->cell = (CELL*)reader_context->cellheader;
        reader->reader_context = NULL;
        return 1;
      }

    } else if (asttype_type == AST_LIST) {
      continue;
    }
  }
  lerror(READER_STATE_ERROR, "unexpectedly reached end of reader loop");
}

void reader_visit(
    READER *reader,
    void *ctx,
    bool (*visit_fn)(READER *reader, CELL *cell, void *ctx)) {

  typedef struct frame {
    CELLHEADER *cellheader;
    uint8_t subcellcounter;
  } FRAME;

  ENVIRONMENT *e = reader->environment;
  LIST *framestack = list_new(e->bs);
  lassert(reader->putc != NULL, READER_STATE_ERROR);

  CELLHEADER *cellheader = (CELLHEADER*)reader->cell;

  while (cellheader) {
    FRAME *parentframe = list_first(framestack);

    if (parentframe != NULL && parentframe->subcellcounter == 0) {
      // parent list is now empty, terminate list and pop cellstack
      visit_fn(reader, (CELL*)parentframe->cellheader, ctx);
      list_shift(framestack);
      parentframe = list_first(framestack);
      if (parentframe) {
        parentframe->subcellcounter--;
      } else {
        cellheader = NULL;
      }
      continue;
    }
    lassert(
      !parentframe || parentframe->subcellcounter > 0,
      READER_STATE_ERROR);

    if (cellheader->Symbol.type == AST_SYMBOL) {
      visit_fn(reader, (CELL*)cellheader, ctx);

      cellheader = (CELLHEADER*)(
        &((CELL*)cellheader)->string[cellheader->Symbol.length]);
      if (parentframe) {
        parentframe->subcellcounter--;
      }

    } else if (cellheader->Integer.type == AST_INTEGER) {
      visit_fn(reader, (CELL*)cellheader, ctx);

      cellheader = &cellheader[1];
      if (parentframe) {
        parentframe->subcellcounter--;
      }

    } else if (cellheader->List.type == AST_LIST) {
      FRAME *newframe = bistack_alloc(e->bs, sizeof(FRAME));
      newframe->subcellcounter = cellheader->List.length;
      newframe->cellheader = cellheader;
      list_unshift(framestack, e->bs, newframe);
      cellheader = ((CELL*)cellheader)->cells;
      parentframe = list_first(framestack);
    }
  }
}

char reader_pprint(READER *reader) {
  ENVIRONMENT *e = reader->environment;
  lassert(reader->putc != NULL, READER_STATE_ERROR);

  uint8_t indent = 0;

  LIST *cellstack = list_new(e->bs);

  typedef struct frame {
    CELLHEADER *cellheader;
    uint8_t subcellcounter;
  } FRAME;

  READER_CONTEXT *reader_context = reader->reader_context;
  CELLHEADER *cellheader = (CELLHEADER*)reader->cell;

  while (cellheader) {
    FRAME *parentcellheader = list_first(cellstack);

    if (parentcellheader != NULL && parentcellheader->subcellcounter == 0) {
      // parent list is now empty, terminate list and pop cellstack
      indent--;
      reader_putc(reader, ')');
      reader_putc(reader, '\n');
      for (uint8_t n=0; n<indent; n++) {
        reader_putc(reader, ' ');
      }
      list_shift(cellstack);
      parentcellheader = list_first(cellstack);
      if (parentcellheader) {
        parentcellheader->subcellcounter--;
      } else {
        cellheader = NULL;
      }
      continue;
    }

    char is_start_of_list = parentcellheader == NULL || (
      parentcellheader->cellheader->List.length ==
        parentcellheader->subcellcounter);

    lassert(
      !parentcellheader || parentcellheader->subcellcounter > 0,
      READER_STATE_ERROR);

    if (cellheader->Symbol.type == AST_SYMBOL) {
      CELL *cell = (CELL*)cellheader;
      if (!is_start_of_list) {
        reader_putc(reader, ' ');
      }

      for (int i=0; i<cellheader->Symbol.length; i++) {
        reader_putc(reader, ((char*)(&cellheader[1]))[i]);
      }

      cellheader = (CELLHEADER*)(
        &((CELL*)cellheader)->string[cellheader->Symbol.length]);
      if (parentcellheader) {
        parentcellheader->subcellcounter--;
      }

    } else if (cellheader->Integer.type == AST_INTEGER) {
      if (!is_start_of_list) {
        reader_putc(reader, ' ');
      }
      reader_putc(reader, cellheader->Integer.sign ? '+' : '-');
      uint16_t value = cellheader->Integer.value;

      // print_zeros switches to true on first non-zero digit
      char print_zeros = 0;
      print_zeros |= putc_tens(reader, value, 10000, FALSE);
      print_zeros |= putc_tens(reader, value, 1000, print_zeros);
      print_zeros |= putc_tens(reader, value, 100, print_zeros);
      print_zeros |= putc_tens(reader, value, 10, print_zeros);
      putc_tens(reader, value, 1, TRUE);

      cellheader = &cellheader[1];
      if (parentcellheader) {
        parentcellheader->subcellcounter--;
      }

    } else if (cellheader->List.type == AST_LIST) {
      reader_putstr(reader, AST_PREFIX_STR(cellheader->List.prefix));
      reader_putc(reader, '\n');
      for (uint8_t n=0; n<indent; n++) {
        reader_putc(reader, ' ');
      }
      reader_putc(reader, '(');

      FRAME *newframe = bistack_alloc(e->bs, sizeof(FRAME));
      newframe->subcellcounter = cellheader->List.length;
      newframe->cellheader = cellheader;
      list_unshift(cellstack, e->bs, newframe);
      cellheader = ((CELL*)cellheader)->cells;

      parentcellheader = list_first(cellstack);
      indent++;
    }
  }
  return 0;
}
