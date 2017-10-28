#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "avl.h"
#include "bistack.h"

static uint16_t max(key_t a, key_t b) {
    return (a > b) ? a : b;
}

int avl_height(AVL_NODE *N) {
  if (N == NULL) {
    return 0;
  }
  return N->height;
}
 
AVL_NODE* avl_new_node(BISTACK *bs, key_t key, value_t value) {
    AVL_NODE* node = (AVL_NODE*)bistack_alloc(bs, sizeof(AVL_NODE));
    node->key   = key;
    node->value = value;
    node->left   = NULL;
    node->right  = NULL;
    node->height = 1;  // new node is initially added at leaf
    return(node);
}
 
// A utility function to right rotate subtree rooted with y
// See the diagram given above.
AVL_NODE *avl_right_rotate(AVL_NODE *y) {
    AVL_NODE *x = y->left;
    AVL_NODE *T2 = x->right;
    
    // Perform rotation
    x->right = y;
    y->left = T2;
    
    // Update heights
    y->height = max(avl_height(y->left), avl_height(y->right))+1;
    x->height = max(avl_height(x->left), avl_height(x->right))+1;
    
    // Return new root
    return x;
}
 
// A utility function to left rotate subtree rooted with x
// See the diagram given above.
AVL_NODE *avl_left_rotate(AVL_NODE *x) {
    AVL_NODE *y = x->right;
    AVL_NODE *T2 = y->left;
        
    // Perform rotation
    y->left = x;
    x->right = T2;
    
    //  Update heights
    x->height = max(avl_height(x->left), avl_height(x->right))+1;
    y->height = max(avl_height(y->left), avl_height(y->right))+1;
    
    // Return new root
    return y;
}
 
// Get Balance factor of node N
int avl_get_balance(AVL_NODE *N) {
  if (N == NULL) {
    return 0;
  }
  return avl_height(N->left) - avl_height(N->right);
}

value_t avl_get(AVL_NODE *node, key_t key) {
    while (node != NULL) {
        if (key == node->key) {
            return node->value;
        } else if (key < node->key) {
            node = node->left;
        } else if (key > node->key) {
            node = node->right;
        }
    }
    return AVL_NOTSET;
}

AVL_NODE* avl_insert(
        AVL_NODE* node,
        BISTACK *bs,
        key_t key,
        value_t value) {
    /* 1.  Perform the normal BST rotation */
    if (node == NULL) {
        return(avl_new_node(bs, key, value));
    }

    if (key < node->key) {
        node->left  = avl_insert(node->left, bs, key, value);
    } else {
        node->right = avl_insert(node->right, bs, key, value);
    }
    
    /* 2. Update height of this ancestor node */
    node->height = max(avl_height(node->left), avl_height(node->right)) + 1;
    
    /* 3. Get the balance factor of this ancestor node to check whether
        this node became unbalanced */
    int balance = avl_get_balance(node);
    
    // If this node becomes unbalanced, then there are 4 cases
    
    // Left Left Case
    if (balance > 1 && key < node->left->key) {
        return avl_right_rotate(node);
    }
    
    // Right Right Case
    if (balance < -1 && key > node->right->key) {
        return avl_left_rotate(node);
    }
    
    // Left Right Case
    if (balance > 1 && key > node->left->key) {
        node->left =  avl_left_rotate(node->left);
        return avl_right_rotate(node);
    }
    
    // Right Left Case
    if (balance < -1 && key < node->right->key) {
        node->right = avl_right_rotate(node->right);
        return avl_left_rotate(node);
    }
    
    /* return the (unchanged) node pointer */
    return node;
}

int avl_array_size(AVL_NODE *root) {
    /**
     * Returns the size of array required to hold the entire tree 
     */
    if (root) {
        return (1 << avl_height(root));
    } else {
        return 0;
    }
}

int avl_to_array(AVL_NODE *p, KEYVALUE* keyvalues) {
    if (p == NULL) {
        return 0;
    }
    
    // a stack describing the path up the current node
    AVL_NODE *path[p->height];

    // a bitstack with 1 indicating the right subtree has been visited
    uint16_t path_dir=0;

    // path stack index (ie: stack pointer)
    uint8_t path_i=0;

    // key counter
    uint16_t k_i = 0;

    // the last row needs to be zero'd
    for (int i=(1 << (p->height-1)); i < (1 << p->height); i++) {
        keyvalues[i].key = 0;
        keyvalues[i].value = NULL;
    }

    do {
        if (!(path_dir & 1)) {
            while (p != NULL) {
                assert(path_i < sizeof(path)/sizeof(void*));
                    path[path_i++] = p;
                path_dir <<= 1;
                k_i = (k_i << 1) + 1;
                p = p->left;
            }
        }
        assert(path_i <= sizeof(path)/sizeof(void*));
        k_i = (k_i-1) >> 1;
        p = path[--path_i];
        path_dir >>= 1;
        
        if (!(path_dir & 1)) {
            // if right not visited, descend to the right
            assert(k_i >= 0 && k_i < (1 << sizeof(path)/sizeof(void*)));
            keyvalues[k_i].key = p->key;
            keyvalues[k_i].value = p->value;
            
            path_dir |= 1;
            if (p->right != NULL) {
                k_i = (k_i << 1) + 2;
                path[path_i++] = p;
                p = p->right;
                path_dir <<= 1;
            }
        }
    } while (path_i>0);
    return 1;
}

static inline int parent(int i) { return (i-1)>>1; }
static inline int right(int i) { return (i<<1) + 2; }
static inline int left(int i) { return (i<<1) + 1; }
void bst_array_traverse(KEYVALUE *keys, int arraysize) {
  /**
   * Traverses a BST arranged as an array 
   */
  assert(0); //not implemented
}

 
#ifdef AVL_TEST
#include <stdio.h>
#include "tests/minunit.h"
int tests_run = 0;

// A utility function to print preorder traversal of the tree.
// The function also prints height of every node
void in_order(AVL_NODE *root) {
  if (root != NULL) {
    in_order(root->left);
    printf("%d ", root->key);
    in_order(root->right);
  }
}

static char *test_basic_avl() {
  AVL_NODE *root = NULL;
  BISTACK *bs = bistack_new(10000);

  /* Constructing tree given in the above figure */
  root = avl_insert(root, bs, 10, NULL);
  root = avl_insert(root, bs, 20, NULL);
  root = avl_insert(root, bs, 30, NULL);
  root = avl_insert(root, bs, 40, NULL);
  root = avl_insert(root, bs, 50, NULL);
  root = avl_insert(root, bs, 25, NULL);
 
  /* The constructed AVL Tree would be
            30
           /  \
         20   40
        /  \     \
       10  25    50
  */
 
  //printf("Pre order traversal of the constructed AVL tree is \n");
  //in_order(root);
  mu_assert("avl tree is not balanced for input", avl_height(root) == 3);
  return 0;
}

static char *test_to_array() {
  const int num_tests = 2000;

  // run a test for avl sizes from 1..2000
  for (int j = 0; j<num_tests; j++) {
    AVL_NODE *root = NULL;
    BISTACK *bs = bistack_new(1000000);
    int total=0;

    for (int i=j; i>0; i--) {
      root = avl_insert(root, bs, i, NULL);
      total += i;
    }

    int arraysize = avl_array_size(root);
    mu_assert("array size not correct", j <= arraysize && arraysize <= (j*2));
    KEYVALUE *keyvalues = (KEYVALUE*)bistack_alloc(bs, arraysize*sizeof(KEYVALUE));
    avl_to_array(root, keyvalues);

    for (int i=0; i< arraysize; i++) {
      total -= keyvalues[i].key;
    }
    if (total != 0) {
      printf("j: %d, total: %d\n", j, total);
    }
    mu_assert("some keys were missed?", total == 0);

    bistack_destroy(bs);
  }

  return 0;
}
  

static char *all_tests() {
  mu_run_test(test_basic_avl);
  mu_run_test(test_to_array);
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
