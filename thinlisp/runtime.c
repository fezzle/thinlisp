#include <stdio.h>
#include <setjmp.h>

#include "defines.h"
#include "runtime.h"

jmp_buf __jmpbuff;

void lerror(uint16_t exctype, char *err, ...) {
  #ifndef LIMITED_ENVIRONMENT
  //printf("ERROR: %s\n", err);
  #endif
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


char *thrown_error_to_string(char err) {
    switch(err) {
    case BISTACK_OUT_OF_MEMORY:
        return "Out of memory";
    case READER_SYNTAX_ERROR:
        return "Reader syntax error";
    case READER_STATE_ERROR:
        return "Reader unepxected state Error";
    case READER_SYNTAX_SPURIOUS_LIST_TERMINATOR:
        return "Reader found spurious list terminator";
    case BISTACK_REWIND_TOO_FAR:
        return "Memory alloc rewind bug";
    case BISTACK_DROPMARK_TOO_FAR:
        return "Memory drop mark bug ";
    default:
        return "Unknown Error";
    }
}
