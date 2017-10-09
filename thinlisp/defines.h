#ifndef MAIN_H
#define MAIN_H

#include <string.h>
#include <stdint.h>

#define DEBUG 1

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef POSIX
#define POSIX
#endif


#define EEPROM_ADDR_TO_PTR(X) ((uint8_t*)X)
#define NVMEM_ADDR_TO_PTR(X) ((uint8_t*)X)



#ifndef PSTR
// not Harvard architecture
#define PGM_START_PTR ((void*)0)

#define PSTR(X) ((char*)(X))
#define strncmp_P strncmp
#define strncpy_P strncpy

#define IS_PGM_PTR(X) (TRUE)
#define IS_NVMEM_PTR(X) (FALSE)
#define SYMBOL_HASH(X) (FALSE)

#define PGM_READ_BYTE(X) (*((uint8_t*)X))
#define NVMEM_READ_BYTE(X) (*((uint8_t*)X))
#define DEREF(X) (\
    IS_PGM_PTR(X) ? PGM_READ_BYTE(X) : \
    IS_NVMEM_PTR(X) ? NVMEM_READ_BYTE(X) : \
    *(uint8_t*)X)
#else

// Havard arch
#define PGM_START_PTR ((void*)100000)

#define IS_PGM_PTR(X) ((X) >= PGM_START_PTR)
#define IS_NVMEM_PTR(X) (FALSE)

#define PGM_READ_BYTE(X) (pgm_read_byte(X))
#define NVMEM_READ_BYTE(X) (nvmem_read(X))
#define DEREF(X) (\
    IS_PGM_PTR(X) ? PGM_READ_BYTE(X) : \
    IS_NVMEM_PTR(X) ? NVMEM_READ_BYTE(X) : \
    (*(uint8_t*)X))
#endif

#define DEREF_SYMBOL(SYM_PTR, SYM_IMMEDIATE) (\
    IS_PGM_PTR(SYM_PTR) ? \
        ((uint8_t*)&SYM_IMMEDIATE)[0]=PGM_READ_BYTE((uint8_t*)SYM_PTR), \
        ((uint8_t*)&SYM_IMMEDIATE)[1]=PGM_READ_BYTE((uint8_t*)(SYM_PTR+1)), \
    : IS_NVMEM_PTR(SYMBOL_PTR) ? \
        ((uint8_t*)&SYM_IMMEDIATE)[0]=NVMEM_READ_BYTE((uint8_t*)SYM_PTR), \
        ((uint8_t*)&SYM_IMMEDIATE)[1]=NVMEM_READ_BYTE((uint8_t*)(SYM_PTR+1)), \
    : ((uint8_t*)&SYM_IMMEDIATE)[0]=*(uint8_t*)(&SYM_PTR), \
      ((uint8_t*)&SYM_IMMEDIATE)[1]=* \
)

void deref_symbol(void *ptr, void *immediate) {
    if (IS_PGM_PTR(ptr)) {
        ((uint8_t*)&immediate)[0] = PGM_READ_BYTE((uint8_t*)ptr + 0);
        ((uint8_t*)&immediate)[1] = PGM_READ_BYTE((uint8_t*)ptr + 1);
    } else if (IS_NVMEM_PTR(ptr)) {
        ((uint8_t*)&immediate)[0] = NVMEM_READ_BYTE((uint8_t*)ptr + 0);
        ((uint8_t*)&immediate)[1] = NVMEM_READ_BYTE((uint8_t*)ptr + 1);
    } else {
        ((uint8_t*)&immediate)[0] = (uint8_t*)ptr + 0;
        ((uint8_t*)&immediate)[1] = (uint8_t*)ptr + 1;
    }
}

#define MAX_LIST_SIZE 255
#define MAX_STRING_SIZE 64

typedef uint8_t list_size_t;
typedef uint8_t string_size_t;
typedef uint8_t symbol_size_t;
typedef uint8_t string_hash_t;
typedef char bool;

#define BREAKPOINT \
  { \
  char c = *(char*)0; \
  }


#endif
