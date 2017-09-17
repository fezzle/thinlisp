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
uint32_t hashstr(char *str, int strlen);
uint8_t hashstr_8(char *str, int strlen);
uint8_t hashstr_8_P(char *str, int strlen);
#endif
