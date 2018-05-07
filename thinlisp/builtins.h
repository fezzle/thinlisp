#include "thinlisp.h"

builtin_fn ADD;
builtin_fn CAR;
builtin_fn SUBTRACT;
builtin_fn CDR;
builtin_fn MULTIPLY;
builtin_fn LAMBDA;
builtin_fn CONS;
builtin_fn DISPLAY;
builtin_fn LIST;
builtin_fn DIVIDE;
builtin_fn NULL__QUESTION_MARK__;
builtin_fn EQUALS;
builtin_fn LENGTH;
builtin_fn MAP;
builtin_fn LIST__REF;
builtin_fn __GREATER_THAN__;
builtin_fn VECTOR__REF;
builtin_fn __LESS_THAN__;
builtin_fn ZERO__QUESTION_MARK__;
builtin_fn IF;
builtin_fn APPLY;
builtin_fn SCHEME;
builtin_fn NOT;
builtin_fn STRING__LENGTH;
builtin_fn REMAINDER;
builtin_fn CADR;
builtin_fn MODULO;
builtin_fn EXPT;
builtin_fn IOTA;
builtin_fn QUOTIENT;
builtin_fn STRING__APPEND;
builtin_fn REVERSE;

builtin_fn resolve_builtin(char *str, uint8_t len);
