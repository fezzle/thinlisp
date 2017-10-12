#ifndef CELL_H
#define CELL_H

#include <stdint.h>

#include "defines.h"
#include "bistack.h"
#include "runtime.h"

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
  uint16_t value_high : 13;
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
        uint16_t value_low;
        char string[0];
        char *string_ptr;
        CELLHEADER cells[0];
        CELLHEADER *cells_ptr;
    };
} CELL;


#define CELLHEADER_IS_INTEGER(X) ((X).Integer.type == AST_INTEGER)
#define CELL_IS_INTEGER(X) ((X).header.Integer.type == AST_INTEGER)

inline char cellheader_is_integer(CELLHEADER *cellheader) {
    return cellheader->Integer.type == AST_INTEGER;
}
inline char cell_is_integer(CELL *cell) {
    return cell->header.Integer.type == AST_INTEGER;
}

/*#define CELL_INTEGER_VAL(X) (\
        (int32_t)( \
            (X).header.Integer.sign \
            ? ((int32_t)(X).header.Integer.value_high << 16) + ((int32_t)(X).value_low) \
            : (-(int32_t)(X).header.Integer.value_high << 16) - ((int32_t)(X).value_low)) \
        )*/
inline int32_t cell_integer_val(CELL &cell) {
    int32_t val = (
        ((int32_t)cell.header.Integer.value_high << 16) + (cell.value_low));
    if (cell.header.Integer.sign == 1) {
        return val;
    } else {
        return -val;
    }
}
    
#define CELLHEADER_IS_LIST(X) ((X).List.type == AST_LIST)
#define CELLHEADER_LIST_LENGTH(X) ((X).List.length)
#define CELLHEADER_LIST_IS_PTR(X) ((X).List.is_ptr)
#define CELL_IS_LIST(X) CELLHEADER_IS_LIST((X).header)
#define CELL_LIST_LENGTH(X) CELLHEADER_LIST_LENGTH((X).header)
#define CELL_LIST_IS_PTR(X) CELLHEADER_LIST_IS_PTR((X).header)
#define CELLHEADER_IS_SYMBOL(X) ((X).Symbol.type == AST_SYMBOL)
#define CELLHEADER_SYMBOL_LENGTH(X) ((X).Symbol.length)
#define CELLHEADER_SYMBOL_IS_PTR(X) ((X).Symbol.is_ptr)
#define CELL_IS_SYMBOL(X) CELLHEADER_IS_SYMBOL((X).header)
#define CELL_SYMBOL_LENGTH(X) CELLHEADER_SYMBOL_LENGTH((X).header)
#define CELL_SYMBOL_IS_PTR(X) CELLHEADER_SYMBOL_IS_PTR((X).header)


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

inline char cell_is_list(CELL *cell) {
    return cell->header.Symbol.type == AST_LIST;
}
inline char cell_is_symbol(CELL *cell) {
    return cell->header.Symbol.type == AST_SYMBOL;
}
inline string_size_t cell_symbol_length(CELL *cell) {
    return cell->header.Symbol.length;
}
inline char cell_symbol_is_ptr(CELL *cell) {
    return cell->header.Symbol.is_ptr;
}

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

CELL *cell_integer_init(CELL *cell, int32_t val) {
    cell->header.Integer.type = AST_INTEGER;
    if (val < 0) {
        val = -val;
        cell->header.Integer.sign = 0;
    } else { 
        cell->header.Integer.sign = 1;
    }
    cell->header.Integer.value_high = (uint16_t)(val >> 16);
    cell->value_low = (uint16_t)val;
    return cell;
}


inline CELLHEADER *cellheader_load(CELLHEADER *src, CELLHEADER *dest) {
    uint8_t *src_bytes = (uint8_t*)src;
    uint8_t *dest_bytes = (uint8_t*)dest;
    dassert(sizeof(CELLHEADER) == 2, COMPILED_SIZE_UNEXPECTED);
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
    return dest;
}


CELL *cell_list_advance(BISTACK *bs, CELL *cellptr);
CELL *cell_load(CELL *src, CELL *dest);

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


typedef char (*get_char_n_ptr)(void *, string_size_t);
typedef struct string_function {
    get_char_n_ptr get_char;
    CELL *arg;
} STRING_DEREF;

char get_pgm_string(void *arg, string_size_t n) {
    return PGM_READ_BYTE((char*)arg + n);
}


#endif