#include "utils.h"

uint32_t fnv_32_buf(void *datap, int datalen, uint32_t offset_basis) {
  /* Adapted from: http://www.isthe.com/chongo/tech/comp/fnv/#FNV-0
  */
  uint32_t hash = offset_basis;
  uint8_t *datapos = (uint8_t*)datap;
  uint8_t *dataend = datapos + datalen;
  while (datapos < dataend) {
    hash ^= *datapos++;
#if defined(NO_FNV_GCC_OPTIMIZATION)
    hash *= (uint32_t)16777619L;
#else
    hash += (hash<<1) + (hash<<4) + (hash<<7) + (hash<<8) + (hash<<24);
#endif
  }
  return hash;
}

uint32_t hashstr(char *str, int strlen) {
  return fnv_32_buf(str, strlen, FNV1_32_INIT);
}

uint16_t hashstr_8(char *str, int strlen) {
  // this reduces to 8 bits hash
  uint32_t hash = fnv_32_buf(str, strlen, FNV1_32_INIT);
  hash = (((hash>>8) ^ hash) & TINY_MASK(8));
  return (uint16_t)hash;
}


inline char q16log2(uint16_t v) {
  /**
   * Returns the log2 of a 16bit num
   */
  char shift;
  char r = 0;
  shift = (v > 0xFF) << 3;
  v >>= shift;
  r |= shift;
  
  shift = (v > 0xF) << 2;
  v >>= shift;
  r |= shift;

  shift = (v > 0x3) << 1;
  v >>= shift;
  r |= shift;
  r |= (v >> 1);
  return r;
}


#ifdef UTILS_TEST
#include <stdio.h>
#include <string.h>
#include "tests/minunit.h"
#include "tests/testdata.h"

int tests_run = 0;

static char * test_hash_to_symbol() {
  int builtin_count = sizeof(sample_builtin_method_names) / sizeof(char*);
  uint32_t hashes[builtin_count];
  for (int i=0; i<builtin_count; i++) {
    hash_t hash = hashstr(sample_builtin_method_names[i],
			   strlen(sample_builtin_method_names[i]));
    hashes[i] = hash_to_symbol(hash);
    for (int j=0; j<i; j++) {
      char buffer[128];
      snprintf(buffer, sizeof(buffer)-1,
	       "hash collision 0x%08x with %s and %s",
	       hashes[i],
	       sample_builtin_method_names[i],
	       sample_builtin_method_names[j]);
      buffer[127] = 0;
      mu_assert(buffer,
		sample_builtin_method_names[i] != sample_builtin_method_names[j]);
    }
  }
  for (int i=0; i<builtin_count; i++) {
    //printf("0x%04x  %s\n", hashes[i], sample_builtin_method_names[i]);
  }
  printf("\n");
  return 0;
}

static char *all_tests() {
  mu_run_test(test_hash_to_symbol);
  mu_run_test(test_bloom);
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
