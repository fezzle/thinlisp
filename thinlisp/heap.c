#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "runtime.h"
#include "heap.h"

void heap_mark(HEAP *heap) {
  /* Stores a mark in the current heap where the freeptr can be rewound */
  void *curptr = heap->ptr;
  MARK *newmark = heap_alloc(heap, sizeof(MARK));
  newmark->next = heap->lastmark;
  newmark->ptr = curptr;
  heap->lastmark = newmark;
}

void heap_rewind(HEAP *heap) {
  /* Stores a mark in the current heap that can be rewound to */
  MARK *mark = heap->lastmark;
  // assert no attempts are made to rewind past start
  assert(mark);
  
  if (mark) {
    heap->ptr = mark->ptr;
    heap->lastmark = mark->next;
  }
} 

void *heap_alloc(HEAP *heap, int size) {
  #ifndef NDEBUG
    assert(heap->ptr + size <= heap->end);
  #else
    longjmp(error, OUT_OF_MEMORY);
  #endif
    
  void *ret = heap->ptr;
  heap->ptr += size;
  return ret;
}

HEAP *heap_new(int size) {
  HEAP *heap = malloc(sizeof(HEAP));
  heap->start = malloc(size);
  heap->ptr = heap->start;
  heap->end = (void*)(heap->start + size);
  heap->lastmark = NULL;
  return heap;
}

void free_heap(HEAP *heap) {
  free(heap->start);
  heap->start = NULL;
  heap->ptr = NULL;
  free(heap);
}


#ifdef HEAP_TEST
#include <stdio.h>
#include "tests/minunit.h"
int tests_run = 0;

static char * test_new_heap() {
   HEAP *heap = heap_new(1024);
   mu_assert("new heap not correct size", heap->end - heap->start == 1024);
   free_heap(heap);
   mu_assert("heap not freed", heap->start == NULL);
   return 0;
}

static char * test_alloc() {
   HEAP *heap = heap_new(1024);
   void *lastptr=NULL;
   for (int i=1; i<43; i++) {
     void *ptr = heap_alloc(heap, i);
     if (lastptr) {
       mu_assert("not correct bytes allocated", ptr - lastptr == (i-1));
     }
     lastptr = ptr;
   }
   return 0;
}

static char *test_mark() {
   HEAP *heap = heap_new(4096);
   char *ptrs[26];
   for (int i='a'; i<='z'; i++) {
     heap_mark(heap);
     ptrs[i-'a'] = heap_alloc(heap, i+1);
     memset(ptrs[i-'a'], i, i);
     ptrs[i-'a'][i] = '\0';
     printf("%c is at: 0x%08x\n", i, ptrs[i-'a']);
   }

   for (int i='z'; i>'a'; i--) {
     // ptrs points to 'aaaa...' 'bbbb...'
     // iterate ptrs and assert 6th character is still original character set
     mu_assert("string is clobbered", ptrs[i-'a'][6] == i);
     heap_rewind(heap);
     void *ptr = heap_alloc(heap, i);
     memset(ptr, 'X', i);
     printf("%c is at: 0x%08x\n", i, ptr);
     printf("%c string: %s\n", i, ptrs[i-'a']);
     mu_assert("string is not clobbered", ptrs[i-'a'][6] == 'X');
   }
   return 0;
}
  

static char *all_tests() {
  mu_run_test(test_new_heap);
  mu_run_test(test_alloc);
  mu_run_test(test_mark);
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
