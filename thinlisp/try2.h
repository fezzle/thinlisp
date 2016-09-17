#include "avl.h"

typedef struct reader {
  BISTACK *bs;
  
  // Symbol String table (as opposed to bound-value table)
  AVL_NODE *symbols; // like obhash/oblist, but avl tree
  AVL_NODE *globalsymbols;
  AVL_NODE *systemsymbols;

  // total strlen of the non-global symbols
  uint16_t total_symbols;
  uint16_t total_strlen;
  uint16_t total_astnodes;
  
  void *streamobj;
  char (*getc)(void *streamobj);
  uint8_t in_comment;
  char ungetbuff[4];
  uint8_t ungetbuff_i;
} READER;

