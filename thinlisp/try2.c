#define try2

#include "defines.h"
#include "reader.h"
/*
static AST_TYPE read_tokenstart(READER *reader) {
  // returns the next token type modifiers and type
  char c = reader_getc(reader);
  char c2 = -1;

  uint8_t prefix;
  
  switch (c) {
  case '"':
    return (AST_TYPE){.type = AST_SYMBOL, .prefix = AST_DOUBLEQUOTE};
  case '\'':
    c2 = reader_getc(reader);
    if (c2 == '#') {
      prefix = AST_QUOTE_HASH;
    } else if (c2 != -1) {
      reader_ungetc(reader, c2);
      prefix = AST_QUOTE;
    }
    break;
  case '`':
    prefix = AST_QUASIQUOTE;
    break;
  case ',':
    c2 = reader_getc(reader);
    if (c2 == '@') {
      prefix = AST_COMMA_AT;
    } else {
      reader_ungetc(reader, c2);
      prefix = AST_COMMA;
    }
    break;
  case '&':
    prefix = AST_AMPERSAND;
    break;
  case '@':
    prefix = AST_AT;
    break;
  case '(':
    return (AST_TYPE){.type = AST_LIST, .prefix = 0};
  case ')':
    return (AST_TYPE){.type = AST_ENDOFLIST, .prefix = 0};
  case ';':
    return (AST_TYPE){.type = AST_COMMENT, .prefix = 0};
  case -1:
    return (AST_TYPE){.type = 0, .prefix = 0};
  default:
    // char must be part of symbol, unget it and return symbol
    reader_ungetc(reader, c);
    return (AST_TYPE){.type = AST_SYMBOL, .prefix = 0};
  }
      
  // if we get here, type has a prefix, next char must be list if '(' else atom
  char c3 = reader_getc(reader);
  if (c3 == '(') {
    return (AST_TYPE){.type = AST_LIST, .prefix = prefix };
  } else if (c3 == -1) {
    if (c2 != -1) { 
      reader_ungetc(reader, c2);
    }
    reader_ungetc(reader, c);
    return (AST_TYPE){.type = 0, .prefix = 0};
  }
  printf("unhandled: %c(%hhx)\n", c, c);
  return (AST_TYPE){.type = 0, .prefix = 0};
}*/
