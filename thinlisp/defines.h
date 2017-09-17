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
