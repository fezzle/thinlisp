#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"

static char *builtin_names[] =
    { "quote", "cond", "if", "and", "or", "while", "lambda", "macro", "label",
      "progn",
      "eq", "atom", "cons", "car", "cdr", "read", "eval", "print",
      "set", "not", "load", "symbolp", "numberp", "+", "-", "*", "/", "<",
      "prog1", "apply", "rplaca", "rplacd", "boundp", "error", "exit", "princ",
      "consp", "assoc" };

typedef struct {
  char *symbol;
  int32_t(*fn)(uint8_t count, ...);
} BUILTIN_DEF;

typedef struct {
  int i;
} EVAL_STATE;

int32_t pop(EVAL_STATE *state) {
  return 0;
}

#define TYPE_INTEGER 1
#define IS_TYPE(X, Y) (X&Y)
#define INT_VALUE(X) (X | ((X&1<<30)<<1))

int32_t math(EVAL_STATE *state) {
  int32_t atom;
  char oper = '*';
  int32_t acc = (oper == '*' || oper == '/') ? 1 : 0;
  
  while ((atom=pop(state))) {
    if (IS_TYPE(atom, TYPE_INTEGER)) {
      if (oper == '*') {
	acc *= INT_VALUE(atom);
      } else if (oper == '/') {
	acc /= INT_VALUE(atom);
      } else if (oper == '+') {
	acc += INT_VALUE(atom);
      } else if (oper == '-') {
	acc -= INT_VALUE(atom);
      }
    }
  }
}

int32_t bool(EVAL_STATE *state) {
  
}

int32_t and(EVAL_STATE *state) {
int32_t atom;
int32_t acc = 1;
while (atom=pop(state) {
acc = ;

}
	 }

BUILTIN_DEF defs = [
		    { .symbol = PSTR("*"), .fn = multiply },
		    { .symbol = PSTR("-"), .fn = subtract },
		    { .symbol = PSTR("+"), .fn = add },
		    { .symbol = PSTR("/"), .fn = divide },
		    { .symbol = PSTR("bind"), .fn = bind },
		    { .symbol = PSTR("
		    ];
