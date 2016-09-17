#include <stdlib.h>

#include "utils.h"
#include "bistack.h"
#include "list.h"

LIST *list_new(BISTACK *bs) {
  LIST* list = (LIST*)bistack_alloc(bs, sizeof(LIST));
  list->head = NULL;
  list->tail = NULL;
  list->count = 0;
  return list;
}

void *list_append(LIST *list, BISTACK *bs, void *val) {
  struct node* newnode = (struct node*)bistack_alloc(bs, sizeof(struct node));
  newnode->val = val;
  newnode->next = NULL;
  if (list->head == NULL) {
    list->head = newnode;
    list->tail = newnode;
  } else {
    list->tail->next = newnode;
    list->tail = newnode;
  }
  list->count++;
  return val;
}

void *list_iter(LIST *list, void **iter) {
  if (*iter == NULL) {
    if (list->head && list->tail) {
      *iter = list->head->next;
      return list->head->val;
    } else {
      return NULL;
    }
  } else {
    void *val = ((struct node*)(*iter))->val;
    *iter = ((struct node*)(*iter))->next;
    return val;
  }
}

void list_reset(LIST *list) {
  list->tail = NULL;
  list->count = 0;
}

int list_count(LIST *list) {
  return list->count;
}


MULTISTRING *ms_new(BISTACK *bs) {
  MULTISTRING *ms = (MULTISTRING*)bistack_alloc(bs, sizeof(MULTISTRING));
  ms->stringbufs.head = NULL;
  ms->stringbufs.tail = NULL;
  ms->totallen = 0;
  ms->bufpos = MULTISTRING_BUFSIZE;
  return ms;
}

MULTISTRING *ms_writechar(MULTISTRING *ms, BISTACK *bs, char ch) {
  char *buf;
  if (ms->bufpos == MULTISTRING_BUFSIZE) {
    // a new char* buffer is required
    buf = bistack_alloc(bs, MULTISTRING_BUFSIZE);
    list_append(&ms->stringbufs, bs, buf);
    ms->bufpos = 0;
  } else {
    buf = ms->stringbufs.tail->val;
  }
  buf[ms->bufpos++] = ch;
  ms->totallen++;
  return ms;
}

hash_t ms_hash(MULTISTRING *ms) {
  unsigned int len = ms->totallen;
  struct node *itr = ms->stringbufs.head;
  // hash initialization value from utils.h
  uint32_t hash = FNV1_32_INIT;

  while (len > 0) {
    int buflen = len > MULTISTRING_BUFSIZE ? MULTISTRING_BUFSIZE : len;
    hash = fnv_32_buf(itr->val, buflen, hash);
    itr = itr->next;
    len -= buflen;
  }
  return hash;
}

char ms_iterchar(MULTISTRING *ms, int *char_index, struct node **buf) {
  if (*char_index >= ms->totallen) {
    return -1;
  }
  int bufpos = *char_index % MULTISTRING_BUFSIZE;
  
  if (*buf == NULL) {
    *char_index = 0;
    bufpos = 0;
    *buf = ms->stringbufs.head;

  } else if (bufpos == 0) {
    *buf = (*buf)->next;
  }
  
  *char_index += 1;
  return ((char*)((*buf)->val))[bufpos];
}

char *ms_assemble(MULTISTRING *ms, char *buff) {
  uint16_t bufpos=0;
  for (struct node* itr=ms->stringbufs.head;
       itr!=NULL;
       itr = itr->next) {
    for (uint16_t itrbufpos=0;
	 itrbufpos < MULTISTRING_BUFSIZE && bufpos < ms->totallen;
	 bufpos++, itrbufpos++) {
      buff[bufpos] = ((char*)itr->val)[itrbufpos];
    }
  }
  buff[bufpos] = 0;
  return buff;
}

int ms_strncmp(MULTISTRING *ms, char *strb, int n) {
  for (struct node* itr=ms->stringbufs.head;
       itr!=NULL;
       itr = itr->next) {
    for (int itrbufpos=0;
	 itrbufpos < MULTISTRING_BUFSIZE && n--;
	 strb++, itrbufpos++) {
      char ms_char = ((char*)itr->val)[itrbufpos];
      if (ms_char < *strb) {
	return -1;
      } else if (ms_char > *strb) {
	return 1;
      }
    }
  }
  return 0;
}


#ifdef LIST_TEST
#include <stdio.h>
#include <string.h>
#include "tests/minunit.h"
#include "tests/testdata.h"
#include "bistack.h"
int tests_run = 0;

static char * test_new_list() {
  BISTACK *bs = bistack_new(1024);
  LIST *list = list_new(bs);
  mu_assert("list->count is not 0", list_count(list) == 0);
  bistack_destroy(bs);
 return 0;
}

static char * test_append_items() {
  BISTACK *bs = bistack_new(1024);
  LIST *list = list_new(bs);
  list_append(list, bs, "first string");
  list_append(list, bs, "second string");
  list_append(list, bs,  "third string");
  list_append(list, bs,  "thing");

  void *itrptr = NULL;
  mu_assert("item (0) mismatch",
	    strcmp(list_iter(list, &itrptr), "first string") == 0);
  mu_assert("item (1) mismatch",
	    strcmp(list_iter(list, &itrptr), "second string") == 0);
  mu_assert("item (2) mismatch",
	    strcmp(list_iter(list, &itrptr), "third string") == 0);
  mu_assert("item (3) mismatch",
	    strcmp(list_iter(list, &itrptr), "thing") == 0);  
  bistack_destroy(bs);
  return 0;
}

static char *test_ms_iter() {
  BISTACK *bs = bistack_new(100 * 1024);
  MULTISTRING *ms = ms_new(bs);

  for (int i=0; i<sizeof(sample_builtin_method_names)/sizeof(char*); i++) {
    for (int j=0; j<strlen(sample_builtin_method_names[i]); j++) {
      ms_writechar(ms, bs, sample_builtin_method_names[i][j]);
      // add data to heap to verify ms_writechar is only writing to correct place
      char *c = bistack_alloc(bs, 1);
      *c = 'X';
    }
  }
  
  int charindex=0;
  struct node *buf = NULL;
  for (int i=0; i<sizeof(sample_builtin_method_names)/sizeof(char*); i++) {
    for (int j=0; j<strlen(sample_builtin_method_names[i]); j++) {
      char c = ms_iterchar(ms, &charindex, &buf);
      mu_assert("char mismatch", sample_builtin_method_names[i][j] == c);
    }
  }
  bistack_destroy(bs);
  return 0;
}

static char *test_ms_assemble() {
  BISTACK *bs = bistack_new(100 * 1024);
  MULTISTRING *ms = ms_new(bs);

  for (int i=0; i<sizeof(sample_builtin_method_names)/sizeof(char*); i++) {
    for (int j=0; j<strlen(sample_builtin_method_names[i]); j++) {
      ms_writechar(ms, bs, sample_builtin_method_names[i][j]);
      // add data to heap to verify ms_writechar is only writing to correct place
      char *c = bistack_alloc(bs, 1);
      *c = 'X';
    }
  }

  char *full_string = ms_assemble(ms, bistack_alloc(bs, ms_length(ms) + 1));
  int charindex=0;
  for (int i=0; i<sizeof(sample_builtin_method_names)/sizeof(char*); i++) {
    for (int j=0; j<strlen(sample_builtin_method_names[i]); j++) {
      mu_assert("char mismatch",
		sample_builtin_method_names[i][j] == full_string[charindex++]);
    }
  }
  
  mu_assert("hash mismatch", ms_hash(ms) == hashstr(full_string, charindex));
  bistack_destroy(bs);
  return 0;
}

static char *all_tests() {
  mu_run_test(test_new_list);
  mu_run_test(test_append_items);
  mu_run_test(test_ms_iter);
  mu_run_test(test_ms_assemble);
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
