#include "eval.h"
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
    }
  }
}

CELLHEADER *lists_equal(
        BISTACK *bs, 
        CELLHEADER *list1, 
        CELLHEADER *list2, 
        list_size_t length) {
    while (length--) {
        CELLHEADER *res = equal(bs, list1, list2);
        if (CELL_IS_FALSE(*res)) {
            return res;
        }
    }
    return CELL_TRUE;
}

CELLHEADER *strings_equal(
        BISTACK *bs, 
        char *str1, 
        char *str2, 
        string_size_t length) {
    while (length--) {
        if (*str1++ != *str2++) {
            return CELL_FALSE;
        }
    }
    return CELL_TRUE;
}

CELLHEADER *equal(BISTACK *bs, CELLHEADER *first, CELLHEADER *second) {
    if (first->wholeheader != second->wholeheader) {
        return CELL_FALSE;
    }
    if (CELL_IS_LIST(*first)) {
        return lists_equal(bs, first+1, second+1, CELL_LIST_LENGTH(*first));
    } else if (CELL_IS_SYMBOL(*first)) {
        return strings_equal(
            bs, (char*)first+1, (char*)second+1, CELL_SYMBOL_LENGTH(*first));
    } else {
        // integer vals are contained in header
        return CELL_TRUE;
    }
}

CELLHEADER *subtract(BISTACK *bs, list_size_t argc, CELLHEADER **argv) {
    int16_t value = 0;
    for (; argc--; argv++) {
        lassert(CELL_IS_INTEGER(**argv), RUNTIME_INTEGER_EXPECTED);
        value -= CELL_INTEGER_VAL(**argv);
    }
    return make_integer(bs, value);
}

CELLHEADER *add(BISTACK *bs, list_size_t argc, CELLHEADER **argv) {
    int16_t value = 0;
    for (; argc--; argv++) {
        lassert(CELL_IS_INTEGER(**argv), RUNTIME_INTEGER_EXPECTED);
        value += CELL_INTEGER_VAL(**argv);
    }
    return make_integer(bs, value);
}
