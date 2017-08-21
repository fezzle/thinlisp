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


void nvmem_fetch(void *dest, code_addr_t src, const size_t len) {
#ifdef POSIX
    lassert(src >= NVMEM_START_ADDRESS, NVMEM_ADDRESS_ERROR);
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


void nvmem_set(code_addr_t dest, void *src, const size_t len) {
#ifdef POSIX
    lassert(dest >= NVMEM_START_ADDRESS, NVMEM_WRITE_ERROR);
    FILE *memfd = fopen("code.mem", "rb+");
    assert_seek(memfd, dest);
    assert_write(memfd, src, len);
    fclose(memfd);
#endif
}


void nvmem_commitblock(code_addr_t addr, NVMEM_BLOCK *block) {
    //  printf("saving block:@%04x next:@%04x size:%04d\n",
    //	 addr, block->nextblock_addr, block->size);
    nvmem_set(addr, block, sizeof(NVMEM_BLOCK));
}


void nvmem_refreshblock(NVMEM_BLOCK *block, code_addr_t addr) {
    nvmem_fetch(block, addr, sizeof(NVMEM_BLOCK));
}


void nvmem_init() {
  char magicbuff[sizeof(MAGIC_WORD)];
  nvmem_fetch(magicbuff, NVMEM_START_ADDRESS, sizeof(magicbuff));

  if (strncmp_P(magicbuff, MAGIC_WORD, sizeof(magicbuff)) != 0) {
    // magic-word not set, need to initialize nvmem
    strncpy_P(magicbuff, MAGIC_WORD, sizeof(magicbuff));
    nvmem_set(NVMEM_START_ADDRESS, magicbuff, sizeof(magicbuff));

    NVMEM_BLOCK freeblock = {
        .free=1,
        .size=NVMEM_END_ADDRESS - sizeof(NVMEM_BLOCK) - NVMEM_START_ADDRESS
    };
    nvmem_commitblock(NVMEM_START_ADDRESS + sizeof(MAGIC_WORD), &freeblock);
  }
}


code_addr_t nvmem_saveblock(void *data, const size_t len) {
    code_addr_t best_addr;
    code_addr_t best_size;
    code_addr_t cur_addr;
    NVMEM_BLOCK block;

    // NOTE: block length includes block header and normalize supplied len
    size_t alloc_size = len + ((1<<BLOCK_ALIGNMENT) - 1) + sizeof(NVMEM_BLOCK);
    alloc_size = alloc_size & ~((1<<BLOCK_ALIGNMENT) - 1);

    cur_addr = NVMEM_START_ADDRESS + sizeof(MAGIC_WORD);
    best_addr = 0;
    best_size = (1<<sizeof(size_t))-1;

    while (cur_addr < NVMEM_END_ADDRESS) {
        nvmem_refreshblock(&block, cur_addr);
        if (block.free && block.size >= alloc_size && best_size > block.size) {
            best_addr = cur_addr;
            best_size = block.size;
        }
        cur_addr += block.size;
    }

    lassert(best_addr != 0, NVMEM_OUT_OF_MEMORY);

    // write the requested data before any FS changes occur just incase process
    //  is interrupted during the most time consuming phase
    nvmem_set(best_addr + sizeof(NVMEM_BLOCK), data, len);

    if (best_size - alloc_size > NVMEM_SPLIT_BLOCK_THRESHOLD) {
        // split block, write used block
        block.size = len;
        block.free = 0;
        nvmem_commitblock(best_addr, &block);

        // compute next block's address
        cur_addr = best_addr + len;

        // load next block and see if we can join next 2 blocks together
        if (best_addr + best_size < NVMEM_END_ADDRESS) {
            nvmem_refreshblock(&block, best_addr + best_size);
        } // else block.free remains =0 as set above
        if (block.free) {
            block.size = (best_addr + best_size - cur_addr + block.size);
            nvmem_commitblock(cur_addr, &block);
        } else {
            block.free = 1;
            block.size = best_addr + best_size - cur_addr;
            nvmem_commitblock(cur_addr, &block);
        }
    } else {
        block.free = 0;
        block.size = best_size;
        nvmem_commitblock(best_addr, &block);
    }

    return best_addr;
}


void nvmem_freeblock(code_addr_t addr) {
    NVMEM_BLOCK tofree;

    // load todelete and prev structs
    nvmem_refreshblock(&tofree, addr);
    tofree.free = 1;
    nvmem_commitblock(addr, &tofree);
}


size_t nvmem_blocksize(code_addr_t addr) {
    NVMEM_BLOCK block;
    nvmem_refreshblock(&block, addr);
    return block.size;
}


void nvmem_loadblock(void *dest, code_addr_t addr) {
    nvmem_fetch(dest, addr + sizeof(NVMEM_BLOCK), nvmem_blocksize(addr));
}


char nvmem_newitr(code_addr_t *addr, NVMEM_BLOCK *block) {
    *addr = NVMEM_START_ADDRESS + sizeof(MAGIC_WORD);
    nvmem_refreshblock(block, *addr);
    return 1;
}

char nvmem_itrnext(code_addr_t *addr, NVMEM_BLOCK *block) {
    code_addr_t nextaddr = *addr + block->size;
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
    code_addr_t addrs[53];
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

    code_addr_t block_addr;
    NVMEM_BLOCK block;
    int nonzero_blocks=0;

    for (char notempty=nvmem_newitr(&block_addr, &block);
            notempty;
            notempty=nvmem_itrnext(&block_addr, &block)) {
        if (block.free == 0 && block.size > 0) {
            nonzero_blocks++;
            char *data = malloc(block.size);
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

    code_addr_t addrs[13];
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

    code_addr_t block0 = nvmem_saveblock(data, 23);
    code_addr_t block1 = nvmem_saveblock(data, 197);
    code_addr_t block2 = nvmem_saveblock(data, 13);
    code_addr_t block3 = nvmem_saveblock(data, 200);
    code_addr_t last_block3;
    code_addr_t last_block2;
    code_addr_t last_block1;
    nvmem_saveblock(data, 99);
    nvmem_saveblock(data, 13);
    nvmem_saveblock(data, 17);
    nvmem_saveblock(data, 135);

    nvmem_freeblock(block3);
    mu_assert("freed block not reused", nvmem_saveblock(data, 198) == block3);

    if (!setjmp(__jmpbuff)) {
        // use up all available memory which causes a longjmp style exception
        code_addr_t last_block1, last_block2, last_block3;
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
     } else {
         printf("ALL TESTS PASSED\n");
     }
     printf("Tests run: %d\n", tests_run);

     return result != 0;
}

#endif
