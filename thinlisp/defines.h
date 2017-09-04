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

#define MAX_LIST_SIZE 255
#define MAX_STRING_SIZE 64

typedef uint8_t list_size_t;
typedef uint8_t string_size_t;
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

#define AST_PREFIX_CHAR1(X) \
  ( \
    (X) == AST_COMMA ? ',' : \
    (X) == AST_AT ? '@' : \
    (X) == AST_COMMA_AT ? ',' : \
    (X) == AST_HASH_QUOTE ? '#' : \
    (X) == AST_HASH ? '#' : \
    (X) == AST_QUOTE ? '\'' : \
    (X) == AST_QUOTE_HASH ? '\'' : \
    (X) == AST_QUASIQUOTE ? '`' : \
    (X) == AST_COMMA ? ',' : \
    (X) == AST_PLUS ? '+' : \
    (X) == AST_MINUS ? '-' : \
    (X) == AST_DOUBLEQUOTE ? '"' : \
    '\0' \
  )

#define AST_PREFIX_CHAR2(X) \
  ( \
    (X) == AST_COMMA_AT ? '@' :	\
    (X) == AST_HASH_QUOTE ? '\'' : \
    (X) == AST_QUOTE_HASH ? '#' : \
    '\0' \
  )

#define AST_POSTFIX_CHAR(X) \
  ((X) == AST_DOUBLEQUOTE ? '"' : '\0')

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

typedef struct {
  uint16_t type : 2;
  uint16_t length : CELL_SYMBOL_LENGTH_BITS;
  uint16_t prefix : CELL_SYMBOL_PREFIX_BITS;
  uint16_t hash : 5;
} SYMBOL;

typedef struct {
  uint16_t type : 2;
  uint16_t sign : 1;
  uint16_t value : 13;
} INTEGER;

typedef struct {
  uint16_t type: 2;
  uint16_t prefix : 4;
  uint16_t length : 10;
} LIST;


typedef union {
  /* Symbol Type */
  SYMBOL Symbol;
  INTEGER Integer;
  LIST List;
  struct {
    uint16_t type : 2;
    uint16_t rest : 14;
  } Common;
  uint16_t wholeheader;
} CELLHEADER;

typedef struct cell {
  CELLHEADER header;
  union {
    char string[0];
    CELLHEADER cells[0];
  };
} CELL;


#define CELL_IS_INTEGER(X) ((X).Integer.type == AST_INTEGER)
#define CELL_INTEGER_VAL(X) (\
    (X).Integer.sign ? (X).Integer.value : -(X).Integer.value \
    )
#define CELL_IS_LIST(X) ((X).List.type == AST_LIST)
#define CELL_LIST_LENGTH(X) ((X).List.length)
#define CELL_IS_SYMBOL(X) ((X).Symbol.type == AST_SYMBOL)
#define CELL_SYMBOL_LENGTH(X) ((X).Symbol.length)

#define CELL_IS_TRUE(X) ( \
    (CELL_IS_INTEGER(X) && CELL_INTEGER_VAL(X) != 0) || \
    (CELL_IS_LIST(X) && CELL_LIST_LENGTH(X) > 0) || \
    (CELL_IS_SYMBOL(X) && CELL_SYMBOL_LENGTH(X) > 0) \
    )

#define CELL_IS_FALSE(X) ( \
    (CELL_IS_INTEGER(X) && CELL_INTEGER_VAL(X) == 0) || \
    (CELL_IS_LIST(X) && CELL_LIST_LENGTH(X) == 0) || \
    (CELL_IS_SYMBOL(X) && CELL_SYMBOL_LENGTH(X) == 0) \
    )

inline CELL *get_cell(CELLHEADER *cellheader) {
  return (CELL*)cellheader;
}



#define BREAKPOINT \
  { \
  char c = *(char*)0; \
  }


#endif
