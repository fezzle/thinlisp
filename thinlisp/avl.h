#ifndef AVL_H
#define AVL_H

#include <stdlib.h>
#include <stdint.h>
#include "bistack.h"
#include "defines.h"

#define AVL_NOTSET NULL

typedef struct avl_node {
  key_t key;
  value_t value;
  struct avl_node *left;
  struct avl_node *right;
  uint8_t height;
} AVL_NODE;

int avl_height(AVL_NODE *n);
AVL_NODE *avl_new_node(BISTACK *bs, key_t key, value_t value);

int avl_array_size(AVL_NODE *n);
int avl_to_array(AVL_NODE *n, KEYVALUE *kvs);

/* A utility function to right rotate subtree rooted with y */
AVL_NODE *avl_right_rotate(AVL_NODE *y);

/* A utility function to left rotate subtree rooted with x */
AVL_NODE *avl_left_rotate(AVL_NODE *x);
 
/* Get Balance factor of node N */
int avl_get_balance(AVL_NODE *N);

value_t avl_get(AVL_NODE *N, key_t key);

/* insert a node in a subtree */
AVL_NODE* avl_insert(AVL_NODE* node,
			    BISTACK *bs,
			    key_t key,
			    value_t value); 
#endif
