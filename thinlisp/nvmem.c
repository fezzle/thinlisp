#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>

#include "nvmem.h"
#include "runtime.h"


void nvmem_initmem() {
#ifdef POSIX  
  FILE *memfd = fopen("code.mem", "wb");
  for (int i=0; i<NVMEM_END_ADDRESS; i++) {
    fwrite("\0", 1, 1, memfd);
  }
  fclose(memfd);
#endif
}
  
  
void nvmem_fetch(void *dest, size_t src, int len) {
#ifdef POSIX
  lassert((size_t)src >= NVMEM_START_ADDRESS, NVMEM_READ_ERROR);
  FILE *memfd = fopen("code.mem", "rb");
  assert_seek(memfd, src);
  assert_read(memfd, dest, len);
  fclose(memfd);
#elif ARDUINO
  while (len--) {
    (*(uint8_t*)dest)++ = pgm_read_byte(addr++);
  }
#endif
}


/** 
 * A private method accepting nvmem offsets and a ptr to memory to write
 */
void nvmem_set(size_t dest, void *src, int len) {
  #ifdef POSIX
  lassert((size_t)dest >= NVMEM_START_ADDRESS, NVMEM_WRITE_ERROR);
  FILE *memfd = fopen("code.mem", "rb+");
  assert_seek(memfd, dest);
  assert_write(memfd, src, len);
  fclose(memfd);
  #endif
}

void nvmem_commitblock(size_t addr, NVMEM_BLOCK *block) {
  //  printf("saving block:@%04x next:@%04x size:%04d\n",
  //	 addr, block->nextblock_addr, block->size);
  nvmem_set(addr, block, sizeof(NVMEM_BLOCK));
}

void nvmem_refreshblock(NVMEM_BLOCK *block, size_t addr) {
  nvmem_fetch(block, addr, sizeof(NVMEM_BLOCK));
}


/**
 * A public method which loads the inmemory fs metadata or creates the fs in
 *  nvmem if it is not currently there.
 * Must be called before any nvmem operations.
 */
void nvmem_init() {  
  char magicbuff[sizeof(MAGIC_WORD)];  
  nvmem_fetch(magicbuff, NVMEM_START_ADDRESS, sizeof(magicbuff));
  
  if (strncmp_P(magicbuff, MAGIC_WORD, sizeof(magicbuff)) != 0) {
    // magic-word not set, need to initialize nvmem
    strncpy_P(magicbuff, MAGIC_WORD, sizeof(magicbuff));
    nvmem_set(NVMEM_START_ADDRESS, magicbuff, sizeof(magicbuff));
    
    NVMEM_BLOCK freeblock = { .free=1,
			      .size_2 = TO_2(NVMEM_END_ADDRESS
					     - sizeof(NVMEM_BLOCK)
					     - NVMEM_START_ADDRESS) };
    nvmem_commitblock(NVMEM_START_ADDRESS + sizeof(MAGIC_WORD), &freeblock);
  }
}


size_t nvmem_saveblock(void *data, size_t len) {
  size_t best_addr;
  size_t best_size;
  size_t cur_addr;
  NVMEM_BLOCK block;

  // NOTE: block length includes block header and normalize supplied len 
  len = FROM_2(TO_2(len + sizeof(NVMEM_BLOCK)));

  cur_addr = NVMEM_START_ADDRESS + sizeof(MAGIC_WORD);
  best_addr = 0;
  best_size = (size_t)-1;

  while (cur_addr < NVMEM_END_ADDRESS) {
    nvmem_refreshblock(&block, cur_addr);
    size_t block_size = FROM_2(block.size_2);
    if (block.free && block_size >= len && best_size > block_size) {
      best_addr = cur_addr;
      best_size = block_size;
    } 
    cur_addr += block_size;  
  }
  
  lassert(best_addr != 0, NVMEM_OUT_OF_MEMORY);

  // write the requested data before any FS changes occur just incase process
  //  is interrupted during the most time consuming phase
  nvmem_set(best_addr + sizeof(NVMEM_BLOCK), data, len - sizeof(NVMEM_BLOCK));

  if (best_size - len > NVMEM_SPLIT_BLOCK_THRESHOLD) {
    // split block, write used block
    block.size_2 = TO_2(len);
    block.free = 0;
    nvmem_commitblock(best_addr, &block);

    // compute next block's address
    cur_addr = best_addr + FROM_2(block.size_2);
    
    // load next block and see if we can join next 2 blocks together
    if (best_addr + best_size < NVMEM_END_ADDRESS) {
      nvmem_refreshblock(&block, best_addr + best_size);
    } // else block.free remains =0 as set above 
    if (block.free) {
      block.size_2 = TO_2((best_addr + best_size - cur_addr)
			  + FROM_2(block.size_2));
      nvmem_commitblock(cur_addr, &block);	
    } else {
      block.free = 1;
      block.size_2 = TO_2(best_addr + best_size - cur_addr);
      nvmem_commitblock(cur_addr, &block);
    }
  } else {
    block.free = 0;
    block.size_2 = TO_2(best_size);
    nvmem_commitblock(best_addr, &block);
  }
  
  return best_addr;
}


void nvmem_freeblock(size_t addr) {
  NVMEM_BLOCK tofree;

  // load todelete and prev structs
  nvmem_refreshblock(&tofree, addr);
  tofree.free = 1;
  nvmem_commitblock(addr, &tofree);
}

/**
 * Accepts a addr and returns the block size.
 * Addr must have been returned by an iterator or by a saveblock call.
 */
size_t nvmem_blocksize(size_t addr) {
  NVMEM_BLOCK block;
  nvmem_refreshblock(&block, addr);
  return FROM_2(block.size_2) - sizeof(NVMEM_BLOCK);
}

/**
 * Accepts a addr and loads the requested data into the destination.
 * The destination must have sufficient space as returned by nvmem_blocksize.
 */
void nvmem_loadblock(void *dest, size_t addr) {
  nvmem_fetch(dest, addr + sizeof(NVMEM_BLOCK), nvmem_blocksize(addr));
}


char nvmem_newitr(size_t *addr, NVMEM_BLOCK *block) {
  *addr = NVMEM_START_ADDRESS + sizeof(MAGIC_WORD);
  nvmem_refreshblock(block, *addr);
  return 1;
}

char nvmem_itrnext(size_t *addr, NVMEM_BLOCK *block) {
  size_t nextaddr = *addr + FROM_2(block->size_2);
  if (nextaddr >= NVMEM_END_ADDRESS) {
    return 0;
  } else {
    *addr = nextaddr;
    nvmem_refreshblock(block, *addr);
    return 1;
  }
}


#ifdef NVMEM_TEST
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include "tests/minunit.h"

int tests_run = 0;

static char * test_block_alloc() {
  nvmem_initmem();
  nvmem_init();

  int count=0;
  size_t addrs[53];
  for (int i=1; i<=52; i++) {
    char *data = malloc(i * 17);
    memset(data, 'X', i * 17 - 1);
    data[i*17 - 1] = 0;
    addrs[i] = nvmem_saveblock(data, i*17);
    if (addrs[i] == 0) {
      break;
    } else {
      count++;
    }
    free(data);
    mu_assert("unable to allocate", addrs[i] != 0);
  }

  size_t block_addr;
  NVMEM_BLOCK block;
  int nonzero_blocks=0;
  
  for (char notempty=nvmem_newitr(&block_addr, &block);
       notempty;
       notempty=nvmem_itrnext(&block_addr, &block)) {
    size_t size = FROM_2(block.size_2);
    if (block.free == 0 && size > 0) {
      nonzero_blocks++;
      char *data = malloc(size);
      nvmem_loadblock(data, block_addr);
      for (char *c=data; *c!=0; c++) {
	mu_assert("character was not 'X'", *c == 'X');
      }
    }
  }
  mu_assert("not enough blocks written", nonzero_blocks == count);
  
  return 0;
}

static char * test_block_alloc_and_contents() {
  nvmem_initmem();
  nvmem_init();
  
  size_t addrs[13];
  for (int i=1; i<13; i++) {
    char *data = malloc(i * 17);
    memset(data, 'X', i * 17 - 1);
    data[i*17 - 1] = 0;
    addrs[i] = nvmem_saveblock(data, i*17);
    free(data);
    mu_assert("unable to allocate", addrs[i] != 0);
  }
  
  for (int i=1; i<13; i++) {
    char *data = malloc(nvmem_blocksize(addrs[i]));
    nvmem_loadblock(data, addrs[i]);
    for (char *c=data; *c!=0; c++) {
      mu_assert("character was not 'X'", *c == 'X');
    }
  }

  return 0;
}


static char * test_block_alloc_and_free() {
  nvmem_initmem();
  nvmem_init();
  char data[200];
  memset(data, 'X', sizeof(data)-1);
  data[sizeof(data)-1] = '\0';

  size_t block0 = nvmem_saveblock(data, 23);
  size_t block1 = nvmem_saveblock(data, 197);
  size_t block2 = nvmem_saveblock(data, 13);
  size_t block3 = nvmem_saveblock(data, 200);
  size_t last_block3;
  size_t last_block2;
  size_t last_block1;
  nvmem_saveblock(data, 99);
  nvmem_saveblock(data, 13);  
  nvmem_saveblock(data, 17);
  nvmem_saveblock(data, 135);  
  
  nvmem_freeblock(block3);
  mu_assert("freed block not reused", nvmem_saveblock(data, 198) == block3);

  if (!setjmp(__jmpbuff)) {
    // use up all available memory which causes a longjmp style exception
    size_t last_block1, last_block2, last_block3;
    while (1) {
      nvmem_freeblock(block2);
      last_block1 = block1;
      last_block2 = block2;
      last_block3 = block3;
      block1 = nvmem_saveblock(data, 197);
      block2 = nvmem_saveblock(data, 187);
      block3 = nvmem_saveblock(data, 163);
    }
  }
  // try to continue using memory after memfull
  mu_assert("no blocks allocated?", last_block3 != 0);
  nvmem_freeblock(block1);
  nvmem_freeblock(block3);
  nvmem_freeblock(block0);
  
  // should be able to allocate new blocks now
  mu_assert("unable to allocate in first place", nvmem_saveblock(data, 23) == block0);
  mu_assert("unable to allocate somewhere in nvmem", nvmem_saveblock(data, 197) == block1);
  mu_assert("unable to allocate at end", nvmem_saveblock(data, 163) == block3);
  return 0;
}


static char *all_tests() {
  mu_run_test(test_block_alloc_and_contents);
  mu_run_test(test_block_alloc);
  mu_run_test(test_block_alloc_and_free);
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
