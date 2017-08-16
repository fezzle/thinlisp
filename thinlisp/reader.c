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
  r->put_missing_reader_context = FALSE;
  r->reader_context = new_reader_context(
    (AST_TYPE){ 
      .type = AST_LIST,
      .prefix = AST_NONE, 
      .terminator = AST_NONE,
    },
    e->bs);
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
      return (AST_TYPE){.type=AST_SYMBOL, .prefix=AST_DOUBLEQUOTE};

    case '\'':
      if (asttype.prefix == AST_HASH) {
        asttype.prefix = AST_HASH_QUOTE;
      } else {
        asttype.prefix = AST_SINGLEQUOTE;
      }
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

    case '+':
      asttype.prefix = AST_PLUS;
      continue;

    case '-':
      asttype.prefix = AST_MINUS;
      continue;

    case '#':
      asttype.prefix = AST_HASH;
      continue;

    case ';':
      lassert(
        asttype.bitfield == AST_NOTYPE.bitfield,
        READER_SYNTAX_ERROR,
        "comment in expression");
      chars_read = 0;
      reader->in_comment = TRUE;
      if (reader_consume_comment(reader) && next_non_ws(reader) != -1) {
        continue;
      } else {
        return AST_NOTYPE;
      }

    case ')':
      return AST_TERMINATOR;

    case ' ':
    case '\n':
      // space/newline encountered while reading start of new cell.  
      // this can occur after reading a 1 character prefix that should be
      //  treated like a symbol (ex: '-' or '+')
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

      } else if ((c >= '"') && (c <= '~')) {
        // all non-control characters are treated like a symbol
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
      continue;

    } else if (symbol_context->is_escaped) {
      // accept character

    } else if (!is_double_quoted &&
        (is_whitespace(c) || (c == ')') || (c == ';'))) {
      // put back character that ends non-double-quoted symbol
      reader_ungetc(reader, c);
      break;

    } else if (is_double_quoted && c == '"') {
      // indicates end iff symbol is double quoted
      finished = is_double_quoted && header->Symbol.length > 0;
      continue;
    }

    if (header->Symbol.length < ((1 << 6) - 1)) {
      ((char*)(&header[1]))[header->Symbol.length++] = c;
      symbol_context->is_escaped = 0;
    }
  }

  header->Symbol.hash = hashstr_8((char*)(&header[1]), header->Symbol.length);

  bistack_allocf(reader->environment->bs, header->Symbol.length);
  return TRUE;
}

/**
 * @return TRUE iff a non-zero digit was printed
 */
char putc_tens(READER *r, uint16_t &value, uint16_t tens, char &print_zero) {
  char c = '0';
  for (; value >= tens; value -= tens) {
    c++;
  }
  if (print_zero || c > '0') {
    return reader_putc(r, c);
  } else { 
    return TRUE;
  }
}


char reader_consume_comment(READER *reader) {
  // reads comment characters until newline
  while (reader->in_comment) {
    char c = reader_getc(reader);
    if (c == -1) {
      return FALSE;
    } else if (c == '\n') {
      reader->in_comment = FALSE;
    }
  }
  return TRUE;
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
    lassert(
      asttype.prefix < (1<<CELL_SYMBOL_PREFIX_BITS), 
      READER_SYNTAX_ERROR, 
      PSTR("Unhandled symbol prefix: %s"), 
      AST_PREFIX_STR(asttype.prefix));
    rc->symbol = bistack_alloc(bs, sizeof(READER_SYMBOL_CONTEXT));
    rc->symbol->is_escaped = 0;
    rc->cellheader = bistack_allocf(bs, sizeof(CELLHEADER));
    rc->cellheader->Symbol.type = asttype.type;
    rc->cellheader->Symbol.prefix = asttype.prefix;
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


void destroy_reader_context(BISTACK *bs, READER_CONTEXT *reader_context) {
    /**
    * Destroys a reader context and asserts the rewind mark matches its mark
    */
    void *rewind_mark = bistack_rewind(bs);
    lassert(rewind_mark == reader_context->rewindmark, READER_STATE_ERROR);
}


bool reader_put_missing(READER *reader) {
    /**
    * calls `reader_putc` with the characters missing to conclude the read.
    */
    lassert(reader->putc != NULL, READER_STATE_ERROR);

    struct put_missing_context {
        READER_CONTEXT *reader_context;
        void *bistack_mark;
    } PUT_MISSING_CONTEXT;

    if (reader->put_missing_context == NULL) {
        // mark and allocate PUT_MISSING_CONTEXT into bistack
        void *bistack_mark = bistack_mark(reader->environment->bs);
        reader->put_missing_context = bistack_alloc(
            reader->environment->bs, sizeof(PUT_MISSING_CONTEXT));
        
        // mark the stack and allocate
        reader->put_missing_context->bistack_mark = bistack_mark;
        reader->put_missing_context->reader_context = (
            reader->reader_context->list->reader_context);
    }

    while (TRUE) {
        READER_CONTEXT *reader_context = (
            reader->put_missing_context->reader_context);

        AST_TYPE asttype = reader_context->asttype;
        char end_char = '?';
        if (asttype.type == AST_SYMBOL) {
            end_char = asttype.prefix == AST_DOUBLEQUOTE ? '"' : '_';
        } else if (asttype.type == AST_INTEGER) {
            end_char = '#';
        } else if (asttype.type == AST_LIST) {
            end_char = ')';
        }

        if (!reader_putc(reader, end_char)) {
            return FALSE;
        } 
        
        if (asttype.type == AST_LIST) {
            // descend into reader_context for sub-list 
            reader->put_missing_context->reader_context = (
                reader_context->list->reader_context);
        } else {
            // current reader_context is not a list - printing is done
            break;
        }
    }

    // rewind stack, assert mark is where we started.
    void *bistack_mark = bistack_rewind(reader->environment->bs);
    lassert(
        bistack_mark == reader->put_missing_context->bistack_mark,
        READER_STATE_ERROR);
    reader->put_missing_context = NULL;
    return TRUE;
}


char reader_read(READER *reader) {
    BISTACK *bs = reader->environment->bs;

    // consume all comments if currently in comment
    if (reader->in_comment) {
        if (reader_consume_comment(reader) == FALSE) {
        return FALSE;
        }
    }

    while (1) {
        // reader_contexts are hierachical through their ->list->reader_context
        READER_CONTEXT *reader_context = reader->reader_context;

        // iterate through heirarchy of reader_contexts to find reader_context and parent
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
                // if no char available while reading next symbol in root context, return TRUE
                // else, a sublist remains unclosed
                return parent_reader_context == reader->reader_context;
                
            } else if (asttype.terminator) {
                // asttype has terminator flag indicated end of list
                lassert(asttype.type == AST_LIST, READER_STATE_ERROR);
                lassert(parent_reader_context != NULL, READER_STATE_ERROR);

                // parent_reader_context list is ending, update parent_parent_reader
                lassert(parent_parent_reader_context != NULL, READER_STATE_ERROR);
                parent_parent_reader_context->cellheader->List.length++;
                parent_parent_reader_context->list->reader_context = NULL;
                destroy_reader_context(bs, parent_reader_context);
                continue;

            } else {
                // new cell
                reader_context = new_reader_context(asttype, bs);
                
                // set parent_reader_context to be currently reading reader_context
                parent_reader_context->list->reader_context = reader_context;
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
            parent_reader_context->cellheader->List.length++;
            parent_reader_context->list->reader_context = NULL;
            destroy_reader_context(bs, reader_context);
            reader_context = NULL;
            continue;

        } else if (asttype_type == AST_LIST) {
            continue;
        }
    }
    lerror(READER_STATE_ERROR, "unexpectedly reached end of reader loop");
}

uint16_t cellheader_character_count(CELLHEADER *cellheader) { 
    if (cellheader->List.type == AST_LIST) {
        uint16_t count = 0;
        for (uint8_t i=0; i<celleader->List.length; i++) {
            count += cellheader_character_count(cellheader);
        }
        return count;
    } else if (cellheader->Integer.type == AST_INTEGER) {
        return 6;
    } else if (cellheader->Symbol.type == AST_SYMBOL) {
        return cellheader->Symbol.length;
    }
}


char reader_pprint(READER *reader) {
    lassert(reader->putc != NULL, READER_STATE_ERROR);

    typedef struct frame {
        // pointer to this frame's CELLHEADER
        CELLHEADER *cellheader;
        // counter counts down list elements, symbol characters or integer tens
        uint8_t counter;
        // if true, this list frame should print a newline after every element
        uint8_t is_newline_per_cell;
    } FRAME;

    typedef struct pprint_context {
        FRAME *topframe;
        LIST *frame_stack;
        void *bistack_mark;
        uint8_t indent_countdown;
        uint8_t is_printing_newline;
        uint8_t indent;
    } PPRINT_CONTEXT;

    const ENVIRONMENT *e = reader->environment;
    const PPRINT_CONTEXT *pprint_context;

    if (reader->pprint_context == NULL) {
        // first call, create context
        void *bistack_mark = bistack_mark(e->bs);
        pprint_context = bistack_alloc(e->bs);
        pprint_context->frame_stack = list_new(e->bs);
        pprint_context->bistack_mark = bistack_mark;
        reader->pprint_context = pprint_context;
        
        // push top frame onto stack
        CELLHEADER *cellheader = reader_context->cellheader;
        FRAME *top_frame = bistack_alloc(e->bs, sizeof(FRAME));
        top_frame->cellheader = cellheader;
        list_unshift(pprint_context->frame_stack, e->bs, top_frame);

    } else {
        // continue from last invocation
        pprint_context = reader->pprint_context;
    }


    while (TRUE) {
        FRAME *frame = list_first(frame_stack);
        FRAME *parent_frame = list_second(frame_stack);

        if (pprint_context->is_printing_newline) {
            if (pprint_context->indent_countdown == 0) {
                // indent_countdown not set yet, print newline
                if (!reader_putc(reader, '\n')) {
                    return FALSE;
                }
                pprint_context->indent_countdown = pprint_context->indent;
            }
            while (pprint_context->indent_countdown) {
                if (!reader_putc(reader, ' ')) {
                    return FALSE;
                }
                pprint_context->indent_countdown--;
            }
            pprint_context->is_printing_newline = FALSE;
        }

        if (frame == NULL) {
            frame = bistack_alloc(e->bs, sizeof(FRAME));
            frame->cellheader = cellheader;
            list_unshift(frame_stack, e->bs, frame);
        }

        if (cellheader->Symbol.type == AST_SYMBOL) {
            CELL *cell = (CELL*)frame->cellheader;
            if (frame->counter == 0) {
                if (frame->prefix_counter == 0) {
                    char prefix1 = AST_PREFIX_CHAR1(cellheader->Symbol.prefix);
                    if (prefix1) {
                        if (!reader_putc(reader, prefix1)) {
                            return FALSE;
                        }
                        frame->prefix_counter++;
                    }
                } 
                if (frame->prefix_counter == 1) {
                    char prefix2 = AST_PREFIX_CHAR2(cellheader->Symbol.prefix);
                    if (prefix2) {
                        if (!reader_putc(reader, prefix1)) {
                            return FALSE;
                        }
                        frame->prefix_counter++;
                    }
                }
                
                frame->counter = frame->cellheader->Symbol.length;
            }
            while (frame->counter > 0) {
                if (!reader_putc(reader, ((char*)(&cellheader[1]))[i])) {
                    return FALSE;
                }
                frame->counter--;
            }
            
            if (!reader_putc(
                        reader, AST_POSTFIX_CHAR(cellheader->Symbol.prefix))) {
                return FALSE;
            }
            
            cellheader = (CELLHEADER*)(
                &((CELL*)cellheader)->string[cellheader->Symbol.length]);
            list_shift(pprint_context->frame_stack);
            
        } else if (cellheader->Integer.type == AST_INTEGER) {
            // print_zeros switches to true on first non-zero digit
            char should_print_zeros = FALSE;
            uint16_t value = cellheader->Integer.value;

            if (frame->counter == 0) {
                frame->counter = 6;
            }
            switch (frame->counter) {
            case 6:
                char sign = cellheader->Integer.sign ? '+' : '-';
                if (!reader_putc(reader, sign)) {
                    return FALSE;
                }
                frame->counter--;
            case 5:
                if (!putc_tens(reader, value, 10000, should_print_zeros)) {
                    return FALSE;
                }
                frame->counter--;
            case 4:
                if (!putc_tens(reader, value, 1000, should_print_zeros)) {
                    return FALSE;
                }
                frame->counter--;
            case 3:
                if (!putc_tens(reader, value, 100, should_print_zeros)) {
                    return FALSE;
                }
                frame->counter--;
            case 2:
                if (!putc_tens(reader, value, 10, should_print_zeros)) {
                    return FALSE;
                }
                frame->counter--;
            case 1:
                if (!putc_tens(reader, value, 1, should_print_zeros)) {
                    return FALSE;
                }
                frame->counter--;
            }
            cellheader = &cellheader[1];
            list_shift(pprint_context->frame_stack);
            
        } else if (cellheader->List.type == AST_LIST) {
            if (frame->counter == 0) {
                // parent list is now empty, terminate list and pop framestack
                if (reader_putc(reader, ')') == FALSE) {
                    return FALSE;
                }
                list_shift(framestack);
                parentcellheader = list_first(framestack);
                if (parentcellheader) {
                    parentcellheader->counter--;
                } else {
                    cellheader = NULL;
                }
                continue;
            }

            if (frame->counter == 0) {
                // todo prefix string state
                reader_putstr(reader, AST_PREFIX_STR(cellheader->List.prefix));
                if (!reader_putc(reader, '(')) {
                    return FALSE;
                }
                frame->counter = cellheader->List.length;
            }
            newframe->is_newline_per_cell = (
                cellheader_character_count(cellheader) > 50);
            list_unshift(framestack, e->bs, newframe);
            cellheader = ((CELL*)cellheader)->cells;

            parentcellheader = list_first(framestack);
            indent++;
        }
    }

    // rewind and assert rewound mark is where this continuation began
    void *bistack_mark = bistack_mark(reader->environment->bs);
    lassert(
        bistack_mark == bistack_rewind(reader->environment->bs), 
        READER_STATE_ERROR);
    reader->pprint_context = NULL;
    return TRUE;
}
