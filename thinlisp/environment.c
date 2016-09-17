#include "avl.h"
#include "bistack.h"
#include "main.h"
#include "utils.c"

typedef struct symboltable {
  AVL_NODE *avlnode;
  // in some sense this is 'prev' as it's one symbol table higher in the env
  struct symboltable *outer;
} SYMBOLTABLE;

typedef struct environment {
  BISTACK *bs;
  char dir;
  SYMBOLTABLE *innerbindings;
  SYMBOLTABLE *globalbindings;
} ENVIRONMENT;

/**
 * Marks a gc point in the bistack and pushes a symbol table in the environment
 */ 
SYMBOLTABLE *env_pushsymtable(ENVIRONMENT *env) {
  bistack_pushdir(env->bs, env->dir);
  bistack_mark(env->bs);
  SYMBOLTABLE *st = bistack_alloc(env->bs, sizeof(SYMBOLTABLE));
  st->avlnode = NULL;
  st->outer = env->innerbindings;
  env->innerbindings = st;
  if (!env->globalbindings) {
    env->globalbindings = st;
  }
  bistack_popdir(env->bs);
  return st;
}

/**
 * Removes the last symbol table from the environment and frees the last mark.
 */
void env_popsymtable(ENVIRONMENT *env) {
  assert(env->innerbindings != NULL);
  
  bistack_pushdir(env->bs, env->dir);
  bistack_rewind(env->bs);
  if (env->innerbindings) {
    env->innerbindings = env->innerbindings->outer;
  }
  bistack_popdir(env->bs);
}

/**
 * Binds a symbol in the environment.
 */
void env_bind_symbol(ENVIRONMENT *env, SYMBOL sym, VALUE v) {
  assert(env->innerbindings != NULL);
  
  bistack_pushdir(env->bs, env->dir);
  env->innerbindings->avlnode = avl_insert(env->innerbindings->avlnode,
					   env->bs, sym, v);
  bistack_popdir(env->bs);
}
