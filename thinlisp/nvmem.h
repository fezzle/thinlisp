#ifndef NVMEM_H
#define NVMEM_H

#include <stdio.h>
#include <stdint.h>
#include "runtime.h"

#define POSIX
#define NVMEM_START_ADDRESS 8000
#define NVMEM_END_ADDRESS (1<<15)
#define MAGIC_WORD PSTR("slimlisp")
#define EEPROM_SIZE (1<<15)
#define NVMEM_SPLIT_BLOCK_THRESHOLD (sizeof(NVMEM_BLOCK)<<1) 

typedef struct {
  // size includes header
  size_t free:1;
  size_t size_2:15;
} NVMEM_BLOCK;

typedef struct {
  uint8_t symbols;
  uint8_t ast;
  uint16_t nextblock_addr;
  uint16_t size;
  uint8_t namelen;
  char name[1];
} NVMEM_MODULEBLOCK;

#define TO_2(X) (((X)+1) >> 1)
#define FROM_2(X) ((X) << 1)

/* Returns the addr of the saved data */
size_t nvmem_saveblock(void *data, size_t len);
void nvmem_freeblock(size_t addr);
void nvmem_loadblock(void *dest, size_t addr);
char nvmem_newitr(size_t *addr, NVMEM_BLOCK *size);
char nvmem_itrnext(size_t *addr, NVMEM_BLOCK *size);

void nvmem_fetch(void *dest, size_t src, int len);
void nvmem_set(size_t dest, void *src, int len);
  
size_t nvmem_blocksize(size_t addr);
void nvmem_init();

inline static void assert_write(FILE *file, void *ptr, size_t size) {
  size_t count = fwrite(ptr, size, 1, file);
  lassert(count == 1, NVMEM_WRITE_ERROR);
}

inline static void assert_read(FILE *file, void *ptr, size_t size) {
  size_t count = fread(ptr, size, 1, file);
  lassert(count == 1, NVMEM_READ_ERROR);
}

inline static void assert_seek(FILE *file, size_t pos) {
  lassert(fseek(file, pos, SEEK_SET) != -1, NVMEM_READ_ERROR);
}


#endif
