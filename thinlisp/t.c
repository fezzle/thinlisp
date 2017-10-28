#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define H1(s,i,x,l)   (x*65599u+(uint8_t)s[(i)<l?l-1-(i):l])
#define H4(s,i,x,l)   H1(s,i,H1(s,i+1,H1(s,i+2,H1(s,i+3,x,l),l),l),l)
#define H16(s,i,x,l)  H4(s,i,H4(s,i+4,H4(s,i+8,H4(s,i+12,x,l),l),l),l)
#define H64(s,i,x,l)  H16(s,i,H16(s,i+16,H16(s,i+32,H16(s,i+48,x,l),l),l),l)
#define H256(s,i,x,l) H64(s,i,H64(s,i+64,H64(s,i+128,H64(s,i+192,x,l),l),l),l)

#define HASH(s,l)    ((uint32_t)(H256(s,0,0,l)^(H256(s,0,0,l)>>16)))


int main() {
  char *str1 = "\0b011001100\x6c";
  char *str2 = "kjhsadfadfalkdf";
  printf("%s, 0x%08x\n", "hello world", HASH("hello world", 11));
  printf("%s, 0x%08x\n", "the quick brown fox", HASH("the quick brown fox", 18));
  printf("%s, 0x%08x\n", "hello world", HASH("hello world", 11));
}
