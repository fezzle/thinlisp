#include "defines.h"
#include "environment.h"
#include "runtime.h"


CELLHEADER *evaluate(CELLHEADER *cell, ENVIRONMENT *env) {
  if (cell->Symbol.type == AST_SYMBOL) {
    if (cell->Symbol.prefix == AST_QUOTE) {
      return cell;
    } else {
      CELLHEADER *value = env_find_symbol(&cell->Symbol);
      lassert(value != NULL, RUNTIME_SYMBOL_IS_UNBOUND);
      return value;
    }

  } else if (cell->Integer.type == AST_INTEGER) {
    return cell;

  } else if (cell->List.type == AST_LIST) {
    if (cell->List.prefix == AST_QUOTE) {
      return cell;
    } else if (cell->List.length == 0) {
      return cell;
    } else {
      CELLHEADER *method = &((CELL*)cell)->cells[0];
      CELLHEADER env_find_symbol();
    }
  }
}

void *add(CELLHEADER *list) {
  int16_t value = 0;
  CELLHEADER *cellitr = list;
  if (list->List.length > 1) {
    for (int i=0; i<list->List.length; i++) {
      cellitr++;
      cellitr->
    }
  }
}

VALUE *new_value(HEAP *heap, int type, void *v_as_str) {
  VALUE *v = (VALUE*)heap_alloc(heap, sizeof(VALUE));
  v->type = type;
  if (type == tSTRING) {
    int vlenplus1 = strlen((char*)v_as_str) + 1;
    v->str = (char*)malloc(vlenplus1);
    strlcpy(v->str, (char*)v_as_str, vlenplus1);
  } else if (type == tINTEGER) {
    v->number = atoi((char*)v_as_str);
  } else if (type == tLIST) {
    v->list = (LIST*)v_as_str;
  } else if (type == tSYMBOL) {
    v->symbol = NULL; //find_symbol((char*)v_as_str);
    if (!v->symbol) {
      v->symbol = new_symbol(heap, (char*)v_as_str, NULL);
    }
  }
  return v;
}
