#ifndef NVMEM_H
#define NVMEM_H

#include <stdio.h>
#include <stdint.h>
#include "runtime.h"
#include "defines.h"

#define POSIX
#define NVMEM_START_ADDRESS 8000
#define NVMEM_END_ADDRESS (1<<15)
#define MAGIC_WORD PSTR("slimlisp")
#define EEPROM_SIZE (1<<15)
#define NVMEM_SPLIT_BLOCK_THRESHOLD (sizeof(NVMEM_BLOCK)<<1)
#define BLOCK_ALIGNMENT 2


typedef size_t code_addr_t;
typedef struct {
  // size includes header
  size_t free:1;
  size_t size:15;
} NVMEM_BLOCK;

typedef struct {
  uint8_t symbols;
  uint8_t ast;
  uint16_t nextblock_addr;
  uint16_t size;
  uint8_t namelen;
  char name[1];
} NVMEM_MODULEBLOCK;


/**
 * Stores data in nvmem
 * @param[in] data A ptr to the data to store
 * @param[in] len The length of data in ptr to store
 * @return The code address of the stored block
 */
code_addr_t nvmem_saveblock(void *data, size_t len);

/**
 * Frees data in nvmem
 * @param[in] addr The address of the block to free.  This is the address
 *     returned by nvmem_saveblock, not the address of the block header.
 */
void nvmem_freeblock(code_addr_t addr);

/**
 * An internal method for loading data from a block to dest.
 * The destination must have sufficient space as returned by nvmem_blocksize.
 * @param[out] dest Address to write block contents to
 * @param[in] addr Address the block is located at
 */
void nvmem_loadblock(void *dest, code_addr_t addr);

/**
 * Creates a new Iterator over all the blocks
 * @param[in] addr Address the block is located at
 * @param[out] dest Address to write block header to
 */
char nvmem_newitr(code_addr_t *addr, NVMEM_BLOCK *block);

/**
 * Loads the next block header after the one identified by *block
 * @param[in] addr Address the block is located at
 * @param[out] dest Address to write block header to
 */
char nvmem_itrnext(code_addr_t *addr, NVMEM_BLOCK *block);

/**
 * An internal method for loading data from code address src into dest
 * @param[out] dest Pointer to destination of loaded data
 * @param[in] src Code address of source data
 * @param[in] len Number of bytes to load
 */
void nvmem_fetch(void *dest, code_addr_t src, const size_t len);

/**
 * An internal method accepting nvmem offsets and a ptr to memory to write.
 * @param[out] dest Code Address to write the desired data to
 * @param[in] src Memory ptr of data to write
 * @param[in] len Length of memory ptr
 */
void nvmem_set(code_addr_t dest, void *src, const size_t len);


/**
 * Accepts a addr and returns the block size.
 * @param[in] addr Code Address of block to return block size.
 * @return The size of the block at addr.
 */
code_addr_t nvmem_blocksize(code_addr_t addr);

/**
 * A public method which loads the inmemory fs metadata or creates the fs in
 *  nvmem if it is not currently there.
 * Must be called before any nvmem operations.
 */
void nvmem_init();


/**
 * Writes to the desired file or assert-fails
 * @param[out] file The FILE ptr to write to
 * @param[in] ptr The data to write
 * @param[in] size The number of bytes to write
 */
inline static void assert_write(FILE *file, void *ptr, const size_t size) {
    size_t count = fwrite(ptr, size, 1, file);
    lassert(count == 1, NVMEM_WRITE_ERROR);
}

/**
 * Readers from the desired file or assert-fails
 * @param[in] file The FILE ptr to read from
 * @param[out] ptr The address to write to
 * @param[in] size The number of bytes to write
 */
inline static void assert_read(FILE *file, void *ptr, const size_t size) {
    size_t count = fread(ptr, size, 1, file);
    lassert(count == 1, NVMEM_READ_ERROR);
}

/**
 * Seeks in the supplied FILE object to the desired address.
 * @param[out] file The FILE ptr to read from
 * @param[in] pos The address to write to.
 */
inline static void assert_seek(FILE *file, code_addr_t pos) {
    lassert(fseek(file, pos, SEEK_SET) != -1, NVMEM_READ_ERROR);
}

#endif
