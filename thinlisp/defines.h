#ifndef MAIN_H
#define MAIN_H

#include <string.h>
#include <stdint.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef POSIX
#define POSIX
#endif

#ifndef PSTR
#define PSTR(X) ((char*)(X))
#define strncmp_P strncmp
#define strncpy_P strncpy
#endif


typedef char bool;

// low 2 bits of AST type are numeration
// high 6 bits are bitfield
#define AST_NONE  0
#define AST_SYMBOL 1
#define AST_INTEGER 2
#define AST_LIST 3
#define AST_STRING 4

enum {
  AST_NOPREFIX=0,
  AST_COMMA, 
  AST_AT,
  AST_COMMA_AT,
  AST_HASH_QUOTE,
  AST_HASH,
  AST_QUOTE,
  AST_DOUBLEQUOTE,
  AST_QUOTE_HASH,
  AST_QUASIQUOTE,
  AST_PLUS,
  AST_MINUS,
};

#define AST_SINGLEQUOTE AST_QUOTE

#define AST_PREFIX_STR(X) \
  ( \
    (X) == AST_COMMA ? PSTR(",") : \
    (X) == AST_AT ? PSTR("@") : \
    (X) == AST_COMMA_AT ? PSTR(",@") :	\
    (X) == AST_HASH_QUOTE ? PSTR("#'") : \
    (X) == AST_HASH ? PSTR("#") : \
    (X) == AST_QUOTE ? PSTR("'") :	\
    (X) == AST_QUOTE_HASH ? PSTR("'#") : \
    (X) == AST_QUASIQUOTE ? PSTR("`") : \
    (X) == AST_COMMA ? PSTR(",") :	\
    (X) == AST_PLUS ? PSTR("+") : \
    (X) == AST_MINUS ? PSTR("-") : \
    (X) == AST_DOUBLEQUOTE ? PSTR("\"") : \
    PSTR("") \
  )
   
  
#define AST_POSTFIX_STR(X) \
  ( \
    (X) == AST_DOUBLEQUOTE ? PSTR("\"") : \
    PSTR("") \
  )

typedef struct ast_type {
  union {
    struct {
      uint8_t type : 2;
      uint8_t prefix : 4;
      uint8_t terminator : 1;
     };
     uint8_t bitfield;
  };
} AST_TYPE;

#define CELL_SYMBOL_PREFIX_BITS 3
#define CELL_SYMBOL_LENGTH_BITS 6

typedef union {
  /* Symbol Type */
  struct {
    uint16_t type : 2;
    uint16_t length : CELL_SYMBOL_LENGTH_BITS;
    uint16_t prefix : CELL_SYMBOL_PREFIX_BITS;
    uint16_t hash : 5;
  } Symbol;

  /* Integer type */
  struct {
    uint16_t type : 2;
    uint16_t sign : 1;
    uint16_t value : 13;
  } Integer;

  /* List Type */
  struct {
    uint16_t type: 2;
    uint16_t prefix : 4;
    uint16_t length : 10;
  } List;

  struct {
    uint16_t type : 2;
    uint16_t rest : 14;
  } Common;
} CELLHEADER;

typedef struct cell {
  CELLHEADER header;
  union {
    char string[0];
    CELLHEADER cells[0];
  };
} CELL;


inline CELL *get_cell(CELLHEADER *cellheader) {
  return (CELL*)cellheader;
}



#define BREAKPOINT \
  { \
  char c = *(char*)0; \
  }


#endif
