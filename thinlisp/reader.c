#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "defines.h"
#include "reader.h"
#include "runtime.h"
#include "cell.h"

ENVIRONMENT *environment_new(BISTACK *bs) {
    ENVIRONMENT *e = bistack_alloc(bs, sizeof(ENVIRONMENT));
    e->bs = bs;
    return e;
}

READER *reader_init(READER *reader) {
    reader->put_missing_context = NULL;
    reader->pprint_context = NULL;
    reader->reader_context = new_reader_context(reader->environment->bs, NULL);
    reader->ungetbuff_i = 0;
    
    reader->putc = NULL;
    reader->putc_streamobj = NULL;

    reader->getc = NULL;
    reader->getc_address = NULL;
    reader->getc_streamobj = NULL;
    
    return reader;
}

READER *reader_new(ENVIRONMENT *e) {
    READER *reader = bistack_alloc(e->bs, sizeof(READER));
    reader->environment = e;
    return reader_init(reader);
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
    while (reader->state == READER_READING_COMMENT) {
        char c = reader_getc(reader);
        if (c == -1) {
            return FALSE;
        } else if (c == '\n') {
            reader->state = READER_READING_NEXT_SYMBOL;
        }
    }
    return TRUE;
}


inline READER_CONTEXT *reader_push_context(READER *reader) {
    /**
    * Marks and allocates a READER_CONTEXT on the default(BACKWARD) stack of BS
    * Allocates a new CELLHEADER on the FORWARD stack of BS.
    */
    BISTACK *bs = reader->environment->bs;
    READER_CONTEXT *reader_context = reader->reader_context;

    void *rewindmark = bistack_heapmark(bs);
    READER_CONTEXT *rc = bistack_heapalloc(bs, sizeof(READER_CONTEXT));
    rc->rewindmark = rewindmark;
    rc->parent = reader_context;
    rc->stack_lock = bistack_lockstack(bs);
    reader->reader_context = rc;
    return rc;
}


inline READER_CONTEXT *reader_pop_context(READER *reader) {
    /**
    * Destroys a reader context and asserts the rewind mark matches its mark
    */
    BISTACK *bs = reader->environment->bs;
    READER_CONTEXT *reader_context = reader->reader_context;

    void *rewind_mark = bistack_heaprewind(bs);
    lassert(rewind_mark == reader_context->rewindmark, READER_STATE_ERROR);
    bistack_unlock(bs, reader_context->stack_lock);

    reader->reader_context = reader_context->parent;
    return reader->reader_context;
}


inline CELL * reader_cell_alloc(READER *reader) {
    return (CELL *)bistack_pushstack(
        reader->environment->bs, 
        reader->reader_context->stack_lock, 
        sizeof(CELL));
}

bool_t reader_read(READER *reader) {
    BISTACK *bs = reader->environment->bs;

    // reader_contexts are hierachical through their ->list->reader_context
    READER_CONTEXT *reader_context = reader->reader_context;
            
    while (reader->state != READER_READING_DONE) {
        // consume all comments if currently in comment
        if (reader->state == READER_READING_COMMENT) {
            if (reader_consume_comment(reader) == FALSE) {
                return FALSE;
            }
        }

        // get next char
        char c = reader_getc(reader);
        if (c == -1) {
            return FALSE;
        }

        // handle next char based on current state
        switch (reader->state) {
            
        case READER_READING_NEGATIVE_INTEGER:
        case READER_READING_INTEGER:
            if ((c & 0b110000) && c >= '0' && c <= '9')  {
                uint32_t v = reader_context->cell->integer.value;
                v = v * 10 + (c - '9');
                if (v && reader->state == READER_READING_NEGATIVE_INTEGER) {
                    // flip sign of integer value once its non-zero
                    reader_context->cell->integer.value = -v;
                    reader->state = READER_READING_INTEGER;
                } else {
                    reader_context->cell->integer.value = v;
                }

            } else if (is_whitespace(c) || c == ')') {
                reader->state = READER_READING_INTEGER_DONE;

            } else {
                lerror(READER_NON_NUMERIC_IN_INTEGER);

            }
            break;


        case READER_READING_PREFIX:
            if (is_whitespace(c)) {
                lerror(READER_WHITESPACE_IN_PREFIX);
            } else if (c == ',') {
                if (reader->reader_context->state != READER_IN_QUASIQUOTE) {
                    lerror(READER_COMMA_ILLEGAL_OUTSIDE_OF_BACKQUOTE);
                }
            }

            reader->state = READER_READING_NEXT_SYMBOL;
            continue;


        case READER_READING_NEXT_SYMBOL:
            if (c == '-') {
                reader->state = READER_READING_NEGATIVE_INTEGER;
                reader_context->cell = reader_cell_alloc(reader);
                cell_init_integer(reader_context->cell, 0);

            } else if (c == '+') {
                reader->state = READER_READING_INTEGER;
                reader_context->cell = reader_cell_alloc(reader);
                cell_init_integer(reader_context->cell, 0);

            } else if (c == '(') {
                reader->state = READER_READING_LIST;
                reader_context->cell = reader_cell_alloc(reader);
                cell_init_list(reader_context->cell, 0);
                reader_context = new_reader_context(bs, reader_context);

            } else if (c == ')') {
                // terminate list
            
            } else if (c == ',') {
                // evaluate next element

            } else if (c == ';') {
                reader->state = READER_READING_COMMENT;
                continue;
            
            } else {
                reader_context->cell = reader_cell_alloc(reader);
                cell_init_symbol(reader_context->cell, 0);

                if (reader->getc_address == NULL) {
                    // can't get address of input stream position, need to 
                    //  store string on stack
                    reader_context->stack_lock = bistack_lockstack(bs);
                    char *buff = bistack_reservestack(
                        bs, reader_context->stack_lock, MAX_STRING_SIZE);

                    buff[0] = c;
                    reader_context->cell->symbol.address = mem_to_addr(bs);
                    reader_context->cell->symbol.length++;
                } else {
                    // can get address
                    reader_context->cell->symbol.address = (
                        reader_getc_address(reader) - 1);
                    reader_context->cell->symbol.length++;
                }
                
                if (c == '"') {
                    reader->state = READER_READING_SYMBOL_DOUBLEQUOTED;
                } else {
                    reader->state = READER_READING_SYMBOL;
                }
            }
            break;
        
        case READER_READING_SYMBOL:
        case READER_READING_SYMBOL_DOUBLEQUOTED:
        case READER_READING_SYMBOL_DOUBLEQUOTED_ESCAPED:
            if (reader->state == READER_READING_SYMBOL) {
                if (is_whitespace(c)) {
                    reader->state = READER_READING_SYMBOL_DONE;
                    break;
                }
            } else if (reader->state == READER_READING_SYMBOL_DOUBLEQUOTED) {
                if (c == '\\') { 
                    reader->state = READER_READING_SYMBOL_DOUBLEQUOTED_ESCAPED;
                    // don't break here, keep backslash in symbol
                } else if (c == '"') {
                    reader->state = READER_READING_SYMBOL_DONE;
                    // don't break here, keep endquote in symbol
                }
            }

            if (reader->getc_address == NULL) {
                // store character in pre-allocated buffer
                char *buff = addr_to_mem(reader_context->cell->symbol.address);
                buff[reader_context->cell->symbol.length] = c;
            } 
            reader_context->cell->symbol.length++;
            break;

        case READER_READING_SYMBOL_DONE:
            if (reader->getc_address == NULL) {
                // move string to heap
                char *buff = bistack_heapalloc(
                    bs, reader_context->cell->symbol.length);
                memcpy(
                    buff, 
                    addr_to_mem(reader_context->cell->symbol.address), 
                    reader_context->cell->symbol.length);
                    reader_context->cell->symbol.address = mem_to_addr(buff);
                bistack_rewindstack(bs);
            }
            
            if (reader_context->parent) {
                reader_context->parent->cell->list.length++;
            }
            reader->state = READER_READING_NEXT_SYMBOL;
            break;
    
        case READER_READING_INTEGER_DONE:
            if (reader_context->parent) {
                reader_context->parent->cell->list.length++;
            }
            reader->state = READER_READING_NEXT_SYMBOL;
            break;

        default:
            lerror(READER_STATE_ERROR);
        }
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