

/* huffman like encoding of AST elements:
 * 0b1..... a 15-bit signed integer
 * 0b01.... a string/symbol
 * 0b00.... a list
 */
#define RDR_IS_INTEGER(X) ((uint16_t)(X) & (0x8000))
#define RDR_IS_SYMBOL(X) (((uint16_t)(X) & (0xC000)) == 0x4000)
#define RDR_IS_LIST(X) (((uint16_t)(X) & (0xC0000)) == 0)

#define STRING_QUALIFIERS_MASK (0x2000)
#define STRING_IS_QUOTED(X) ((X) & 0x2000)
#define STRING_SET_QUOTED(X) ((X) | 0x2000)

#define LIST_QUALIFIERS_MASK (0x2000)
#define LIST_IS_QUOTED(X) ((X) & (0x2000))



int16_t rdr_make_integer(int16_t n) {
  if ((int16_t)n < -BIT14) {
    return BIT15 | -1;
  } else if ((int16_t)n >= BIT14) {
    return BIT15 | (BIT14 - 1);
  } else {
    return BIT15 | n;
  }
}

int16_t rdr_make_string(char *str, int len, int qualifiers) {
  assert((qualifiers & ~STRING_QUALIFIERS_MASK) == 0);
  return BIT14 | qualifiers | hashstr_13(str, len);
}

int16_t rdr_make_list(int len, int qualifiers) {
  assert((qualifiers & ~LIST_QUALIFIERS_MASK) == 0);
  return qualifiers | len;
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
