/* file minunit_example.c */
 
#include <stdio.h>
#include "minunit.h"
#include "../reader.h"
 
int tests_run = 0;


static char * test_ls_append() {
   HEAP *heap = new_heap(1024);
   LIST *list = ls_new(heap);
   ls_append(heap, list, "apple");
   ls_append(heap, list, "orange");
   ls_append(heap, list, "pear");

   void *itr=NULL;
   mu_assert("not apple", !strcmp(ls_itr(list, &itr), "apple"));
   mu_assert("not orange", !strcmp(ls_itr(list, &itr), "orange"));
   mu_assert("not pear", !strcmp(ls_itr(list, &itr), "pear"));
   return 0;
}

static char * test_ls_reset_with_one() {
   HEAP *heap = new_heap(1024);
   LIST *list = ls_new(heap);
   ls_append(heap, list, "apple");
   ls_reset(list);
   ls_append(heap, list, "pear");
   ls_append(heap, list, "grapefruit");
   ls_append(heap, list, "blueberry");

   void *itr=NULL;
   mu_assert("not pear", !strcmp(ls_itr(list, &itr), "apple"));
   mu_assert("not grapefruit", !strcmp(ls_itr(list, &itr), "orange"));
   mu_assert("not blueberry", !strcmp(ls_itr(list, &itr), "pear"));
   return 0;
}


static char *all_tests() {
  mu_run_test(ls_append);
  mu_run_test(test_bar);
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
