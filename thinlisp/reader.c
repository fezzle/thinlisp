#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "reader.h"
#include "runtime.h"

READER *reader_new(BISTACK *bs) {
  READER *r = bistack_alloc(bs, sizeof(READER));
  r->bs = bs;
  r->root_list = NULL;
  r->current_list = NULL;
  r->current_atom = NULL;
  r->ungetbuff_i = 0;
  r->in_comment = 0;
  r->streamobj = NULL;

  r->total_symbols = 0;
  r->total_strlen = 0;
  r->total_astnodes = 0;
  return r;
}
 

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
}


SYMBOL reader_makeintegerorsymbol(READER *reader) {
  /**
   * Returns a pointer to an AST_NODE on the backward bistack if ms is an
   * Integer.
   *
   * The symbol must be fully read into reader->current_atom->string
   */
  assert(reader->current_atom);
  assert(reader->current_atom->string);
  assert(bistack_dir(reader->bs) == BS_FORWARD);
  
  // MARKB the read symbol
  bistack_markb(reader->bs);

  // assemble buffer for symbol
  int strlen = ms_length(reader->current_atom->string) + 1;
  char *str = bistack_allocb(reader->bs, strlen);
  ms_assemble(reader->current_atom->string, str);
  symbol_t symbol = hashstr(str, strlen);

  // try integer if AST_SYMBOL type with no prefixes
  if (reader->current_atom->type.type == AST_SYMBOL
      && reader->current_atom->type.prefix == 0) {
    char *endptr;
    long val = strtol(str, &endptr, 0);
    if (*endptr == '\0') {
      // fully parsed as integer
      // Stack: rewind the stack to free the assembled multistring 
      bistack_rewindb(reader->bs);
      return ((symbol_t)val) | (THING_INT);
    }
  }
  
  // try to find symbol in existing reader environment
  do {
    AVL_NODE *existing = avl_get(reader->obhash, symbol);
    if (existing == AVL_NOTSET) {
      existing = avl_get(reader->global_obhash, symbol);
    }
    if (existing != AVL_NOTSET) {
      // if symbol exists, confirm its the same string
      // MARKB existing symbol value in hash bucket
      char *existing_str = load_str(reader->bs, existing->value);
      if (strncmp(existing_str, str, strlen) != 0) {
	// not same string, hash collision, increment and check again
	// REWINDB existing symbol value in hash bucket
	bistack_rewindb(reader->bs);
	symbol += 1;
	continue;
      } else {
	// same string, use this one
	// REWINDB existing symbol value in hash bucket
	bistack_rewindb(reader->bs);
	// REWINDB the read symbol
	bistack_rewindb(reader->bs);
	return symbol;
      }
    } else {
      // DORPMARKB the read symbol - new symbol string must be stored
      bistack_dropmarkb(reader->bs);
      bistack_pushdir(reader->bs, BS_BACKWARD);
      reader->obhash = avl_insert(reader->obhash, reader->bs, symbol, str);
      bistack_popdir(reader->bs);
      return symbol;
    }
  } while (1);

  // Stack: was marked earlier, but the full string is now required. 
  bistack_dropmarkb(reader->bs);

  // make new symbol
  reader->symbols = avl_insert(reader->symbols, reader->bs, symbol, str);
  AST_NODE *astnode = bistack_allocb(reader->bs, sizeof(AST_NODE));
  astnode->type = reader->current_atom->type;
  astnode->symbol = symbol;
  reader->total_astnodes += 1;
  reader->total_strlen += symbollen;
  return astnode;
}


AST_NODE *read_atom(READER *reader) {
  /**
   * Reads or continues reading an atom from reader.
   * Marks the stack on entrance, rewinds the stack once an atom is fully read.
   */
  MULTISTRING *ms = reader->current_atom->string;
  AST_TYPE atomtype = reader->current_atom->type;

  while (1) {
    char c = reader_getc(reader);
    if (c == -1) {
      return NULL;
    }
      
    if (c == '\\' && !reader->current_atom->escaped) {
      // mark the next character as escaped
      reader->current_atom->escaped = 1;

    } else if (reader->current_atom->escaped) {
      // write this character regardless of what it is
      ms_writechar(ms, reader->bs, c);
      reader->current_atom->escaped = 0;

    } else if ((c == '\"' && atomtype.prefix == AST_DOUBLEQUOTE)
	       || (atomtype.prefix != AST_DOUBLEQUOTE && (
		    (c == ' ') || (c == '\t') || (c == '\n')
		    || ( c == ')') || ( c == '(')))) {
      if (c == ')' || c == '(') {
	reader_ungetc(reader, c);
      }
      
      bistack_pushdir(reader->bs, BS_BACKWARD);
      AST_NODE *ast_node =  reader_makeintegerorsymbol(reader);
      bistack_popdir(reader->bs);
      return ast_node;
      
    } else {
      ms_writechar(ms, reader->bs, c);
    }
  }
}

 
AST_NODE *reader_continue(READER *reader) {
  while (1) {
    // if in comment, loop until newline
    while (reader->in_comment) {
      char c = reader_getc(reader);
      if (c == -1) {
	printf("\nEOF in comment?");
	return NULL;
      } else if (c == '\n') {
	reader->in_comment = 0;
      }
    }
      
    // read current atom
    if (reader->current_atom) {
      AST_NODE *val = read_atom(reader);
      if (val) {
	printf("endatom rewindf\n");
	bistack_rewindf(reader->bs);
	reader->current_atom = NULL;
	if (reader->current_list) {
	  // atom is part of a current list, append and continue
	  list_append(reader->current_list->list, reader->bs, val);
        } else {
	  // atom appears alone, return it  
	  return val;
	}
	printf("AST_LIST(0x%08x) <- AST_ATOM(0x%08x)  read atom symbol:%d prefix:%s val:%s\n",
	       (int)reader->current_list->list,
	       (int)val, val->symbol,
	       AST_PREFIX_STR(val->type), 
	       avl_get(reader->symbols, val->symbol));
      } else {
	printf(" no val! \n");
	return NULL;
      }
    }

    if (next_non_ws(reader) == -1) {
      return NULL;
    }
    
    // read next token
    AST_TYPE type = read_tokenstart(reader);

    if (type.type == AST_SYMBOL || type.type == AST_INTEGER) {
      assert(bistack_dir(reader->bs) == BS_FORWARD);
      // F stack: mark and allocate buffer for reading this atom
      bistack_markf(reader->bs);
      reader->current_atom = bistack_allocf(reader->bs, sizeof(READER_ATOM_CONTEXT));
      reader->current_atom->string = ms_new(reader->bs);
      reader->current_atom->type = type;

    } else if (type.type == AST_COMMENT) {
      reader->in_comment = 1;
      
    } else if (type.type == AST_LIST) {
      // ASSERT bistack is going forward
      assert(bistack_dir(reader->bs) == BS_FORWARD);
      bistack_markf(reader->bs);
      printf("newlist markf \n");
      
      READER_LIST_CONTEXT *ctx = bistack_allocf(reader->bs,
	sizeof(READER_LIST_CONTEXT));

      ctx->type = type;
      ctx->list = list_new(reader->bs);
      ctx->next = NULL;
      
      if (reader->root_list == NULL) {
        reader->root_list = ctx;
        reader->current_list = ctx;
      } else {
        reader->current_list->next = ctx;
        reader->current_list = ctx;
      }
      
    } else if (type.type == AST_ENDOFLIST) {
      assert(bistack_dir(reader->bs) == BS_FORWARD);
      
      uint8_t listlen = list_count(reader->current_list->list);
      // Stack B: allocate all AST_NODE data on backward stack
      AST_NODE *val = bistack_allocb(reader->bs, sizeof(AST_NODE));
      val->type = reader->current_list->type;
      val->length = listlen;
      val->astvector = bistack_allocb(reader->bs, sizeof(void*) * listlen);
      LIST_NODE *listnodeptr = reader->current_list->list->head;
      for (char i=0; i<listlen; i++) {
	assert(listnodeptr != NULL);
	val->astvector[i] = listnodeptr->val;
	listnodeptr = listnodeptr->next;
      }

      // rewind reader's current list context 
      READER_LIST_CONTEXT *ptr = reader->root_list;

      // Stack F: free this current-list context, all relevant data should now
      //  be in *val
      printf("endlist rewindf \n");
      bistack_rewindf(reader->bs);

      if (ptr->next == NULL) {
	reader->root_list = NULL;
      } else {
	while (ptr->next != reader->current_list) {
	  ptr = ptr->next;
	}
	// append this AST_NODE list to parent list
	list_append(ptr->list, reader->bs, val);
	printf("AST_LIST(0x%08x) <- AST_LIST(0x%08x)  read list length: %d\n",
	       (int)(ptr->list), (int)val, val->length);
	
	ptr->next = NULL;
	reader->current_list = ptr;
      }
      
      // if this was root list, return val
      if (reader->root_list == NULL) {
	return val;
      }
    } else {
      lerror(0, PSTR("syntax error"));
    }
  }
}


void reader_pprint(READER *reader, AST_NODE *root, uint8_t indentspaces) {
  for (char i=indentspaces; i-->0;) {
    putchar(' ');
  }
  printf("%s", AST_PREFIX_STR(root->type));

  if (root->type.type == AST_SYMBOL) {
    char *str = avl_get(reader->symbols, root->symbol);
    if (str == AVL_NOTSET) {
      str = avl_get(reader->globalsymbols, root->symbol);
    }
    if (str == AVL_NOTSET) {
      str = PSTR("str not found");
    }
    printf("%s%s", str, AST_POSTFIX_STR(root->type));

  } else if (root->type.type == AST_INTEGER) {
      printf("%d", root->intval);
      
  } else if (root->type.type == AST_LIST) {
    putchar('(');
    for (int i=0; i<root->length; i++) {
      if (root->astvector[i]->type.type == AST_LIST) {
	putchar('\n');
	reader_pprint(reader, root->astvector[i], indentspaces + 2);
	for (char i=indentspaces; i-->0;) {
	  putchar(' ');
	}
      } else {
	reader_pprint(reader, root->astvector[i], 0);
	if (i+1 < root->length) {
	  putchar(' ');
	}
      }
    }
    putchar(')');
    putchar('\n');
  }
}




#ifdef READER_TEST
#include <stdio.h>
#include <string.h>
#include "tests/minunit.h"

int tests_run = 0;

char mygetc(void *streamobj) {
  int c = fgetc((FILE*)streamobj);
  if (c == EOF) {
    return -1;
  } else {
    return (char)c;
  }
}

static char * test_reader_sample() {
   BISTACK *bs = bistack_new(8096);
   READER *reader = reader_new(bs);
   reader_setio(reader, mygetc);
   reader->streamobj = fopen("tests/sample.lisp", "rb");

   AST_NODE *astnode = reader_continue(reader);
   
   mu_assert("astnode should not be null", astnode != NULL);
   reader_pprint(reader, astnode, 0);
   
   return 0;
}



static char *all_tests() {
  mu_run_test(test_reader_sample);
  return 0;
}

int main(int argc, char **argv) {
     char *result = all_tests();
     if (result != 0) {
         printf("%s\n", result);
     }
     else {
         printf("ALL TESTS PASSED\n");
     }
     printf("Tests run: %d\n", tests_run);
 
     return result != 0;
}

#endif
