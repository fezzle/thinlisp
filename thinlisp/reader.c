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

READER *reader_init(READER *reader) {
    reader->in_comment = FALSE;
    reader->put_missing_context = NULL;
    reader->pprint_context = NULL;
    reader->reader_context = new_reader_context(
        (AST_TYPE){
            .type = AST_LIST,
            .prefix = AST_NONE,
            .terminator = AST_NONE,
        },
        reader->environment->bs);
    reader->ungetbuff_i = 0;
    reader->putc_streamobj = NULL;
    reader->putc = NULL;
    reader->getc_streamobj = NULL;
    reader->getc = NULL;
    return reader;
}

READER *reader_new(ENVIRONMENT *e) {
    READER *reader = bistack_alloc(e->bs, sizeof(READER));
    reader->environment = e;
    return reader_init(reader);
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
            return (AST_TYPE){
                .type=AST_SYMBOL,
                .prefix=AST_NONE,
                .terminator=FALSE
            };
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
                    .terminator=FALSE
                };

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

    header->Symbol.hash = hashstr_8(
        (char*)(&header[1]),
        header->Symbol.length);

    bistack_allocf(reader->environment->bs, header->Symbol.length);
    return TRUE;
}

/**
 * @return TRUE iff a non-zero digit was printed
 */
char putc_tens(READER *r, uint16_t* value, uint16_t tens, char* print_zero) {
    char c = '0';
    for (; *value >= tens; *value -= tens) {
        c++;
    }
    if (*print_zero || c > '0') {
        *print_zero = TRUE;
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
    * Marks and allocates a READER_CONTEXT on the default(BACKWARD) stack of BS
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
        cell_list_init(rc->cellheader, asttype.prefix, FALSE, 0);
        break;
    case AST_SYMBOL:
        lassert(
            asttype.prefix < (1<<CELL_SYMBOL_PREFIX_BITS),
            READER_SYNTAX_ERROR,
            PSTR("Unhandled symbol prefix: %c%c"),
            AST_PREFIX_CHAR1(asttype.prefix),
            AST_PREFIX_CHAR2(asttype.prefix));
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

    typedef struct {
        READER_CONTEXT *reader_context;
    } FRAME;

    typedef struct {
        LIST *frame_stack;
        void *start_mark;
    } PUT_MISSING_CONTEXT;

    BISTACK *bs = reader->environment->bs;
    PUT_MISSING_CONTEXT *context = NULL;

    if (reader->reader_context->list->reader_context == NULL) {
        // nothing to read
        return TRUE;
    }

    if (reader->put_missing_context == NULL) {
        // mark and allocate PUT_MISSING_CONTEXT into bistack
        void *start_mark = bistack_mark(reader->environment->bs);
        reader->put_missing_context = bistack_alloc(
            reader->environment->bs, sizeof(PUT_MISSING_CONTEXT));

        context = (PUT_MISSING_CONTEXT*)reader->put_missing_context;
        context->start_mark = start_mark;
        context->frame_stack = list_new(bs);
        FRAME *frame = bistack_alloc(bs, sizeof(FRAME));
        frame->reader_context = reader->reader_context->list->reader_context;
        list_unshift(context->frame_stack, bs, frame);
        // populate frame-stack with reader_contexts
        while (TRUE) {
            if (frame->reader_context->asttype.type == AST_LIST
                    && frame->reader_context->list->reader_context != NULL) {
                FRAME *new_frame = bistack_alloc(bs, sizeof(FRAME));
                new_frame->reader_context = (
                    frame->reader_context->list->reader_context);
                list_unshift(context->frame_stack, bs, new_frame);
                frame = new_frame;
            } else {
                break;
            }
        }
    } else {
        context = (PUT_MISSING_CONTEXT*)reader->put_missing_context;
    }

    while (list_first(context->frame_stack)) {
        FRAME *frame = (FRAME*)list_first(context->frame_stack);
        READER_CONTEXT *reader_context = frame->reader_context;
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
        list_shift(context->frame_stack);
    }

    // rewind stack, assert mark is where we started.
    void *start_mark = bistack_rewind(reader->environment->bs);
    lassert(start_mark == context->start_mark, READER_STATE_ERROR);
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

        // iterate through heirarchy of reader_contexts to find reader_context
        //   and parent
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
                // If no char available while reading next symbol in root
                //  context, return TRUE.
                // Else, a sublist remains unclosed, return FALSE
                return parent_reader_context == reader->reader_context;

            } else if (asttype.terminator) {
                // asttype has terminator flag indicated end of list
                lassert(asttype.type == AST_LIST, READER_STATE_ERROR);
                lassert(parent_reader_context != NULL, READER_STATE_ERROR);

                // parent_reader_context list is ending,
                //   update parent_parent_reader
                lassert(
                    parent_parent_reader_context != NULL,
                    READER_SYNTAX_SPURIOUS_LIST_TERMINATOR);
                parent_parent_reader_context->cellheader->List.length++;
                parent_parent_reader_context->list->reader_context = NULL;
                destroy_reader_context(bs, parent_reader_context);
                continue;

            } else {
                // new cell
                reader_context = new_reader_context(asttype, bs);

                // set parent_reader_context to be currently reading
                //  reader_context
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
    uint8_t frame_stack[16] = {0};
    int8_t frame_i = -1;
    uint16_t char_count = 0;

    do {
        if (cellheader->List.type == AST_LIST) {
            char_count += 4;
            frame_stack[++frame_i] = cellheader->List.length;
            cellheader = &cellheader[1];

        } else if (cellheader->Integer.type == AST_INTEGER) {
            char_count += 6;
            frame_stack[frame_i]--;
            cellheader = &cellheader[1];

        } else if (cellheader->Symbol.type == AST_SYMBOL) {
            char_count += 2 + cellheader->Symbol.length;
            frame_stack[frame_i]--;
            cellheader = (CELLHEADER*)(
                (void*)&cellheader[1] + cellheader->Symbol.length);
        }

        while (frame_i > 0 && frame_stack[frame_i] == 0) {
            frame_stack[--frame_i]--;
        }
    } while (frame_stack[0] > 0);

    return char_count;
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
        // counts number of prefix characters printed
        uint8_t prefix_counter;
    } FRAME;

    typedef struct pprint_context {
        LIST *frame_stack;
        void *start_mark;
        uint8_t indent_countdown;
        uint8_t is_printing_newline;
        uint8_t indent;
    } PPRINT_CONTEXT;

    ENVIRONMENT *e = reader->environment;
    PPRINT_CONTEXT *pprint_context = NULL;
    char c = '\0';

    if (reader->pprint_context == NULL) {
        // first call, create context
        void *start_mark = bistack_mark(e->bs);
        pprint_context = bistack_alloc(e->bs, sizeof(PPRINT_CONTEXT));
        pprint_context->frame_stack = list_new(e->bs);
        pprint_context->start_mark = start_mark;
        reader->pprint_context = pprint_context;

        // push top frame onto stack
        FRAME *frame = bistack_alloc(e->bs, sizeof(FRAME));
        frame->cellheader = reader->reader_context->cellheader;
        frame->counter = 0;
        list_unshift(pprint_context->frame_stack, e->bs, frame);

    } else {
        // continue from last invocation
        pprint_context = (PPRINT_CONTEXT*)reader->pprint_context;
    }

    while (TRUE) {
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
        FRAME *frame = list_first(pprint_context->frame_stack);
        FRAME *parent_frame = list_second(pprint_context->frame_stack);

        CELLHEADER *cellheader = frame->cellheader;
        if (cellheader->Symbol.type == AST_SYMBOL) {
            switch (frame->prefix_counter) {
            case 0:
                c = AST_PREFIX_CHAR1(cellheader->Symbol.prefix);
                if (c && !reader_putc(reader, c)) {
                    return FALSE;
                }
                frame->prefix_counter++;
            case 1:
                c = AST_PREFIX_CHAR2(cellheader->Symbol.prefix);
                if (c && !reader_putc(reader, c)) {
                    return FALSE;
                }
                frame->prefix_counter++;
                frame->counter = frame->cellheader->Symbol.length;
            }
            while (frame->counter) {
                uint8_t i = frame->cellheader->Symbol.length - frame->counter;
                if (!reader_putc(reader, ((char*)(&cellheader[1]))[i])) {
                    return FALSE;
                }
                frame->counter--;
            }
            c = AST_POSTFIX_CHAR(cellheader->Symbol.prefix);
            if (c && !reader_putc(reader, c)) {
                return FALSE;
            }
        } else if (cellheader->Integer.type == AST_INTEGER) {
            // print_zeros switches to true on first non-zero digit
            char should_print_zeros = FALSE;
            uint16_t value = cellheader->Integer.value;

            if (frame->counter == 0) {
                frame->counter = 6;
            }
            switch (frame->counter) {
            case 6:
                c = cellheader->Integer.sign ? '+' : '-';
                if (!reader_putc(reader, c)) {
                    return FALSE;
                }
                frame->counter--;
            case 5:
                if (!putc_tens(reader, &value, 10000, &should_print_zeros)) {
                    return FALSE;
                }
                frame->counter--;
            case 4:
                if (!putc_tens(reader, &value, 1000, &should_print_zeros)) {
                    return FALSE;
                }
                frame->counter--;
            case 3:
                if (!putc_tens(reader, &value, 100, &should_print_zeros)) {
                    return FALSE;
                }
                frame->counter--;
            case 2:
                if (!putc_tens(reader, &value, 10, &should_print_zeros)) {
                    return FALSE;
                }
                frame->counter--;
            case 1:
                if (!putc_tens(reader, &value, 1, &should_print_zeros)) {
                    return FALSE;
                }
                frame->counter--;
            }
        } else if (cellheader->List.type == AST_LIST) {
            switch (frame->prefix_counter) {
            case 0:
                c = AST_PREFIX_CHAR1(cellheader->List.prefix);
                if (c && !reader_putc(reader, c)) {
                    return FALSE;
                }
                frame->prefix_counter++;
            case 1:
                c = AST_PREFIX_CHAR2(cellheader->List.prefix);
                if (c && !reader_putc(reader, c)) {
                    return FALSE;
                }
                frame->prefix_counter++;
            case 2:
                if (!reader_putc(reader, '(')) {
                    return FALSE;
                }
                frame->prefix_counter++;
            case 3:
                if (!reader_putc(reader, ' ')) {
                    return FALSE;
                }
                frame->prefix_counter++;
                frame->counter = frame->cellheader->List.length;
            }

            list_shift(pprint_context->frame_stack);
            frame->counter--;
        }

        while ((frame = list_shift(pprint_context->frame_stack))) {
            frame->counter--;
            if (frame->counter > 0) {
                break;
            }
        }
    }

    // rewind and assert rewound mark is where this continuation began
    lassert(
        pprint_context->start_mark == bistack_rewind(reader->environment->bs),
        READER_STATE_ERROR);
    reader->pprint_context = NULL;
    return TRUE;
}


#ifdef READER_MAIN
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

#endif