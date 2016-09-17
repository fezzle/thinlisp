#ifndef MAIN_H
#define MAIN_H

#include <string.h>
#include <stdint.h>
#include "utils.h"

#ifndef PSTR
#define PSTR(X) ((char*)(X))

#define strncmp_P strncmp
#define strncpy_P strncpy

#endif


//bistack.c debug
//#define BS_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#define BS_DEBUG(...)

#define THING_INT (1<<15)
#define THING_TYPE (1<<14)
#define THING_SYMBOL (0<<14)

typedef union {
  struct {
    uint8_t type : 2;
    uint8_t other : 7;
  };
  struct {
    uint8_t symoltype : 2;
    uint8_t symbolmod : 7;
  };
  struct {
    uint8_t listtype : 2;
    uint8_t listmod : 2;
    uint8_t listlen : 5;
  };
} THING_MODIFIER;

#define SYMBOL_MOD_DBLQUOTE 0
#define SYMBOL_MOD_QUOTE 1
#define SYMBOL_MOD_COMMA 2
#define LIST_MOD 

#define Q16_BE(X) (((uint16_t)(X)>>8) | ((uint16_t)(X)<<8))

// hashes from util's string hash are used as avl keys
typedef hash_t key_t;

// avl keys are used as symbol identifiers
typedef hash_t SYMBOL;

// values are generic ptrs
typedef void* value_t;
typedef value_t VALUE;

typedef struct keyvalue_t {
  key_t key;
  value_t value;
} KEYVALUE;

// low 2 bits of AST type are numeration
// high 6 bits are bitfield
#define AST_SYMBOL ((uint8_t) 1)
#define AST_INTEGER (AST_SYMBOL + 1)
#define AST_LIST (AST_SYMBOL + 2)
#define AST_ENDOFLIST (AST_SYMBOL + 3)
#define AST_COMMENT (AST_SYMBOL + 4)

#define AST_AT ((uint8_t) 1)
#define AST_DOUBLEQUOTE (AST_AT + 1)
#define AST_QUASIQUOTE (AST_AT + 2)
#define AST_QUOTE (AST_AT + 3)
#define AST_COMMA (AST_AT + 4)
#define AST_AMPERSAND (AST_AT + 5)
#define AST_COMMA_AT (AST_AT + 6)
#define AST_QUOTE_HASH (AST_AT + 7)

static const char AST_1PREFIX[] = { 'X', '@', '\"', '`', '\'', ',', '&', 0, 0 };
#define AST_2PREFIX_OFFSET (sizeof(AST_1PREFIX) / sizeof(char))
static const char AST_2PREFIX[][2] = { {'X', 'X'},
				       { ',', '@'},
				       {'\'', '#' } };

#define AST_PREFIX_STR(X)				\
  ((X).prefix == AST_AT ? PSTR("@") :			\
   (X).prefix == AST_DOUBLEQUOTE ? PSTR("\"") :			\
   (X).prefix == AST_QUASIQUOTE ? PSTR("`") :			\
   (X).prefix == AST_COMMA ? PSTR(",") :			\
   (X).prefix == AST_AMPERSAND ? PSTR("&") :			\
   (X).prefix == AST_COMMA_AT ? PSTR(",@") :			\
   (X).prefix == AST_QUOTE_HASH ? PSTR("'#") : PSTR(""))

#define AST_POSTFIX_STR(X) ((X).prefix == AST_DOUBLEQUOTE ? PSTR("\"") : PSTR(""))	
static inline SYMBOL next_symbol(SYMBOL sym) {
  // high bit of symbol is reserved
  return (sym+1) & ~(1 << (sizeof(SYMBOL)-1));
}

typedef struct ast_type {
  union {
    struct {
      uint8_t type : 4;
      uint8_t prefix : 4;
     };
     uint8_t bitfield;
  };
} AST_COMPLEXTYPE;
  

#define BREAKPOINT \
  { \
  char c = *(char*)0; \
  }

typedef struct ast_node {
  AST_TYPE type;
  union {
    struct {
      struct ast_node **astvector;
      uint16_t length;
    };
    int32_t intval;
    SYMBOL symbol;
  };
} AST_NODE;

#endif
