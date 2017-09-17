#ifndef CELL_H
#define CELL_H

#include <stdint.h>

#include "defines.h"

// low 2 bits of AST type are numeration
// high 6 bits are bitfield
#define AST_NONE  0
#define AST_SYMBOL 1
#define AST_INTEGER 2
#define AST_LIST 3
#define AST_STRING 4

#define SYMBOL_HASH_MASK 0b01111

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
#define CELL_SYMBOL_MASK_HASH(X) (X&0b011111)


typedef struct {
  uint16_t type : 2;
  uint16_t length : CELL_SYMBOL_LENGTH_BITS;
  uint16_t prefix : CELL_SYMBOL_PREFIX_BITS;
  uint16_t is_ptr : 1;
  uint16_t hash : 4;
} SYMBOL;


typedef struct {
  uint16_t type : 2;
  uint16_t sign : 1;
  uint16_t value : 13;
} INTEGER;


typedef struct {
  uint16_t type: 2;
  uint16_t prefix : 4;
  uint16_t is_ptr : 1;
  uint16_t length : 9;
} LIST;


/**
 * A cell header only defining the cell metadata for Symbol and List.
 * Contains the value for integer.
 */
typedef union {
  
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
    char *string_ptr;
    CELLHEADER cells[0];
    CELLHEADER *cells_ptr;
  };
} CELL;


#define CELLHEADER_IS_INTEGER(X) ((X).Integer.type == AST_INTEGER)
#define CELL_IS_INTEGER(X) ((X).header.Integer.type == AST_INTEGER)
#define CELL_INTEGER_VAL(X) (\
    (X).Integer.sign ? (X).Integer.value : -(X).Integer.value \
    )

#define CELL_IS_LIST(X) ((X).header.List.type == AST_LIST)
#define CELLHEADER_IS_LIST(X) ((X).List.type == AST_LIST)
#define CELL_LIST_LENGTH(X) ((X).List.length)

#define CELL_IS_SYMBOL(X) ((X).header.Symbol.type == AST_SYMBOL)
#define CELLHEADER_IS_SYMBOL(X) ((X).Symbol.type == AST_SYMBOL)
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

#define CELL_SYMBOL_PREFIX(X) ((X).Symbol.prefix)
#define CELL_SYMBOL_HASH(X) ((X).Symbol.hash)
#define CELL_SYMBOL_CHAR(X, C_I) (((CELL*)&X)->string[C_I])


#define CELLHEADER_LIST_PREFIX(X) ((X).List.prefix)
#define CELL_LIST_PREFIX(X) ((X).header.List.prefix)

CELLHEADER *cell_list_init(
        CELLHEADER *cell, uint8_t prefix, bool is_ptr, list_size_t length) {
    cell->List.type = AST_LIST;
    cell->List.prefix = prefix;
    cell->List.is_ptr = is_ptr;
    cell->List.length = length;
    return cell;
}

int8_t cell_symbol_compare(CELL *a, CELL *b);

CELLHEADER *cell_integer_init(CELLHEADER *cell, int16_t val) {
    cell->Integer.type = AST_INTEGER;
    if (val > 0) {
        cell->Integer.value = val;
        cell->Integer.sign = 1;
    } else {
        cell->Integer.value = -val;
        cell->Integer.sign = 0;
    }
    return cell;
}

/**
 * TODO: update if CELLHEADER size changes
 */
inline CELL *get_cell(CELLHEADER *dest, CELLHEADER *src) {
    uint8_t *src_bytes = (uint8_t*)src;
    uint8_t *dest_bytes = (uint8_t*)dest;
    if (IS_PGM_PTR(src)) {
        dest_bytes[0] = PGM_READ_BYTE(src_bytes);
        dest_bytes[1] = PGM_READ_BYTE(src_bytes + 1);
    } else if (IS_NVMEM_PTR(src)) {
        dest_bytes[0] = NVMEM_READ_BYTE(src_bytes);
        dest_bytes[1] = NVMEM_READ_BYTE(src_bytes + 1);
    } else {
        dest_bytes[0] = src_bytes[0];
        dest_bytes[1] = src_bytes[1];
    }

    if (CELL_IS_LIST(*src)) {
        dest->List.is_ptr = TRUE;
        if (src->List.is_ptr) {
            ((CELL*)dest)->cells_ptr = ((CELL*)src)->cells_ptr;
        } else {
            ((CELL*)dest)->cells_ptr = ((CELL*)src)->cells;
        }
    } else if (CELL_IS_SYMBOL(*src)) {
        dest->Symbol.is_ptr = TRUE;
        if (src->Symbol.is_ptr) {
            ((CELL*)dest)->string_ptr = ((CELL*)src)->string_ptr;
        } else {
            ((CELL*)dest)->string_ptr = ((CELL*)src)->string;
        }
    }
    return (CELL*)dest;
}

inline CELLHEADER *cell_list_get_cells(CELLHEADER *cellheader) {
    /**
     * @param cellheader A cellheader known to be a list type
     * @return a pointer to cells contained by list
     */
    if (cellheader->List.is_ptr) {
        return ((CELL*)cellheader)->cells_ptr;
    } else {
        return cellheader + 1;
    }
}



#endif