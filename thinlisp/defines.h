#ifndef MAIN_H
#define MAIN_H

#include <string.h>
#include <stdint.h>

#ifndef PSTR
#define PSTR(X) ((char*)(X))
#define strncmp_P strncmp
#define strncpy_P strncpy
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef char bool;

// low 2 bits of AST type are numeration
// high 6 bits are bitfield
#define AST_NONE  0
#define AST_SYMBOL 1
#define AST_INTEGER 2
#define AST_LIST 3
#define AST_STRING 4

#define AST_NOPREFIX 0
#define AST_AT ((uint8_t) 1)
#define AST_DOUBLEQUOTE (AST_AT + 1)
#define AST_QUASIQUOTE (AST_AT + 2)
#define AST_QUOTE (AST_AT + 3)
#define AST_COMMA (AST_AT + 4)
#define AST_AMPERSAND (AST_AT + 5)
#define AST_COMMA_AT (AST_AT + 6)
#define AST_QUOTE_HASH (AST_AT + 7)
#define AST_SINGLEQUOTE (AST_AT + 8)
#define AST_MINUS (AST_AT + 9)
#define AST_PLUS (AST_AT + 10)

static const char AST_1PREFIX[] = { 'X', '@', '\"', '`', '\'', ',', '&', 0, 0 };
#define AST_2PREFIX_OFFSET (sizeof(AST_1PREFIX) / sizeof(char))
static const char AST_2PREFIX[][2] = { {'X', 'X'},
				       { ',', '@'},
				       {'\'', '#' } };

#define AST_PREFIX_STR(X)				\
  ((X) == AST_AT ? PSTR("@") :			\
   (X) == AST_DOUBLEQUOTE ? PSTR("\"") :			\
   (X) == AST_QUASIQUOTE ? PSTR("`") :			\
   (X) == AST_COMMA ? PSTR(",") :			\
   (X) == AST_AMPERSAND ? PSTR("&") :			\
   (X) == AST_COMMA_AT ? PSTR(",@") :			\
   (X) == AST_QUOTE_HASH ? PSTR("'#") : PSTR(""))

#define AST_POSTFIX_STR(X) ((X).prefix == AST_DOUBLEQUOTE ? PSTR("\"") : PSTR(""))

#define BIT31 ((int32_t)1<<31)
#define BIT30 ((int32_t)1<<30)
#define BIT15 ((int16_t)1<<15)
#define BIT14 ((int16_t)1<<14)

//bistack.c debug
//#define BS_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#define BS_DEBUG(...)

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

#define AST_NOTYPE ((AST_TYPE) {\
  .type = AST_NONE, \
  .prefix = AST_NONE, \
  .terminator = AST_NONE, \
  })

#define AST_TERMINATOR ((AST_TYPE){\
  .type = AST_NONE,\
  .prefix = AST_NONE,\
  .terminator = TRUE,\
  })

typedef union {
  /* Symbol Type */
  struct {
    uint16_t type : 2;
    uint16_t length : 6;
    uint16_t hash : 8;
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

