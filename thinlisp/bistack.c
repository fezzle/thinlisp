#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "runtime.h"
#include "heap.h"
#include "bistack.h"


void *bistack_allocf(BISTACK *bs, uint16_t size) {
  if (bs->forwardptr + size > bs->backwardptr) {
    lerror(BISTACK_OUT_OF_MEMORY, PSTR("bistack_allocf"));
  }

  void *ptr = bs->forwardptr;
  bs->forwardptr += size;

  BS_DEBUG("bistack_allocf(%d) forwardptr: 0x%08x\n", size, (int)bs->forwardptr);
  return ptr;
}

void *bistack_allocb(BISTACK *bs, uint16_t size) {
  if (bs->forwardptr + size > bs->backwardptr) {
    lerror(BISTACK_OUT_OF_MEMORY, PSTR("bistack_allocb"));
  }

  bs->backwardptr = bs->backwardptr - size;
  BS_DEBUG("bistack_allocb(%d) backwardptr: 0x%08x\n", size, (int)bs->backwardptr);
  return bs->backwardptr;
}

void *bistack_alloc(BISTACK *bs, uint16_t size) {
  if ((bs->direction_stack & 1) == BS_FORWARD) {
    return bistack_allocf(bs, size);
  } else { // bs->default_direction=BS_BACKWARD
    return bistack_allocb(bs, size);
  }
}


BISTACK *bistack_new(int size) {
  BISTACK *bs = malloc(size);
  bs->forwardptr = (void*)bs + sizeof(BISTACK);
  bs->forwardmark = NULL;
  
  bs->backwardptr = (void*)bs + size;
  bs->backwardmark = NULL;

  bs->direction_stack = BS_FORWARD;
  return bs;
}

void bistack_destroy(BISTACK *bs) {
  free(bs);
}

int bistack_freemem(BISTACK *bs) {
  return (int)bs->backwardptr - (int)bs->forwardptr;
}

void bistack_mark(BISTACK *bs) {
  if ((bs->direction_stack & 1) == BS_FORWARD) {
    bistack_markf(bs);
  } else {
    bistack_markb(bs);
  }
}

void bistack_rewind(BISTACK *bs) {
  if ((bs->direction_stack & 1) == BS_FORWARD) {
    bistack_rewindf(bs);
  } else {
    bistack_rewindb(bs);
  }
}

void bistack_markf(BISTACK *bs) {
  /* Stores a mark in the current heap where the freeptr can be rewound */
  BS_DEBUG("bistack_markf @ forwardptr: 0x%08x\n", (int)bs->forwardptr);
  void *curptr = bs->forwardptr;
  MARK *newmark = bistack_allocf(bs, sizeof(MARK));
  newmark->next = bs->forwardmark;
  newmark->ptr = curptr;
  bs->forwardmark = newmark;
}

void bistack_rewindf(BISTACK *bs) {
  /* Pops the stack to the most recent mark */
  MARK *mark = bs->forwardmark;
  // assert no attempts are made to rewind past start
  assert(mark);
  
  bs->forwardptr = mark->ptr;
  bs->forwardmark = mark->next;
  BS_DEBUG("bistack_rewindf @ forwardptr: 0x%08x\n", (int)bs->forwardptr);
} 

void bistack_markb(BISTACK *bs) {
  /* Stores a mark in the current heap where the freeptr can be rewound */
  BS_DEBUG("bistack_markb @ backwardptr: 0x%08x\n", (int)bs->backwardptr);
  void *curptr = bs->backwardptr;
  MARK *newmark = bistack_allocb(bs, sizeof(MARK));
  newmark->next = bs->backwardmark;
  newmark->ptr = curptr;
  bs->backwardmark = newmark;
}

void bistack_rewindb(BISTACK *bs) {
  /* Stores a mark in the current stack that can be rewound to */
  MARK *mark = bs->backwardmark;
  // assert no attempts are made to rewind past start
  assert(mark);
  bs->backwardptr = mark->ptr;
  bs->backwardmark = mark->next;
  BS_DEBUG("bistack_rewindb @ backwardptr: 0x%08x\n", (int)bs->forwardptr);
} 

void bistack_dropmark(BISTACK *bs) {
  if ((bs->direction_stack & 1) == BS_FORWARD) {
    return bistack_dropmarkf(bs);
  } else {
    return bistack_dropmarkb(bs);
  }
}
void bistack_dropmarkf(BISTACK *bs) {
  assert(bs->forwardmark);
  bs->forwardmark = bs->forwardmark->next;
  BS_DEBUG("bistack_dropmarkf @ forwardptr: 0x%08x\n", (int)bs->forwardptr);
}
void bistack_dropmarkb(BISTACK *bs) {
  assert(bs->backwardmark);
  bs->backwardmark = bs->backwardmark->next;
  BS_DEBUG("bistack_dropmarkb @ backwardptr: 0x%08x\n", (int)bs->backwardptr);
}



#ifdef BISTACK_TEST
#include <stdio.h>
#include <string.h>
#include "tests/minunit.h"

int tests_run = 0;

static char * test_new_bistack() {
   BISTACK *bs = bistack_new(1024);
   mu_assert("new bistack not correct size",
	     bistack_freemem(bs) == (1024-4*sizeof(void*)));
   bistack_destroy(bs);
   return 0;
}

static char * test_alloc() {
  BISTACK *bs = bistack_new(1024);
  bistack_markf(bs);
  bistack_markb(bs);
  int freemem = bistack_freemem(bs);
  bistack_rewindf(bs);
  bistack_rewindb(bs);

  char charb='!';
  char charf='"';
  for (int i=1; i<freemem-1; i++) {
    bistack_markf(bs);
    bistack_markb(bs);
    mu_assert("freemem has changed", bistack_freemem(bs) == freemem);
    printf("freemem: %d\n", freemem);
    
    char *ptrf = bistack_allocf(bs, i);
    char *ptrb = bistack_allocb(bs, freemem-i);

    // set f memory area to charf
    memset(ptrf, charf, i);
    ptrf[i-1] = '\0';

    // set b memory area to charb
    memset(ptrb, charb, freemem-i);
    ptrb[freemem-i-1] = '\0';

    printf("ptrf: %s\n", ptrf);
    printf("ptrb: %s\n", ptrb);
    
    // validate ptrf strlen and verify last character is charf
    mu_assert("forwardptr's string len wrong", strlen(ptrf) == i-1);
    if (strlen(ptrf) > 0) {
      mu_assert("forwardptr's char got clobbered", ptrf[i-1-1] == charf);
    }

    // set f memory area to charf again just incase this clobbers part of ptrb
    memset(ptrf, charf, i);
    ptrf[i-1] = '\0';

    // validate ptrb strlen and verify first character of charb is right
    mu_assert("backwardptr's string len wrong", strlen(ptrb) == freemem-i-1);
    if (strlen(ptrb) > 0) {
      mu_assert("backwardptr's char got clobbered", ptrb[0] == charb);
    }
    
    charb = charf;
    charf = ((charf+1)%(127-33)+33);

    bistack_rewindf(bs);
    bistack_rewindb(bs);
  }
  return 0;
}

static char *test_alloc_toomuch() {
  BISTACK *bs = bistack_new(1024);
  int exctype = setjmp(__jmpbuff);
  if (exctype == 0) {
    bistack_allocf(bs, 700);
    bistack_allocb(bs, 700);
    mu_assert("oom not thrown", 0);
  } else {
    mu_assert("exception type was not oom", exctype == OUT_OF_MEMORY);
  }
  return 0;
}

static char *all_tests() {
  mu_run_test(test_new_bistack);
  mu_run_test(test_alloc);
  mu_run_test(test_alloc_toomuch);
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
