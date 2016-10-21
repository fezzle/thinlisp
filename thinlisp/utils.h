#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "defines.h"

#define BIT31 ((int32_t)1<<31)
#define BIT30 ((int32_t)1<<30)
#define BIT15 ((int16_t)1<<15)
#define BIT14 ((int16_t)1<<14)


#define TINY_MASK(X) (((uint32_t)1<<(X))-1)
#define MASK_16 (((uint32_t)1<<16)-1) /* i.e., (u_int32_t)0xffff */
#define FNV1_32_INIT ((uint32_t)2166136261L)

uint32_t fnv_32_buf(void *datap, int datalen, uint32_t offset_basis);
hash_t hashstr(char *str, int strlen);
uint16_t hashstr_13(char *str, int strlen);

static inline hash_t hash_to_symbol(hash_t v) {
  return (v>>16) ^ (v & TINY_MASK(16));
}

#endif
