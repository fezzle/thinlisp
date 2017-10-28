//
//  eeprom.h
//  thinlisp
//
//  Created by Eric on 2016-09-11.
//  Copyright © 2016 norg. All rights reserved.
//

#ifndef eeprom_h
#define eeprom_h

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "defines.h"
#include "runtime.h"

#define NOBLOCK_ADDR 0

#define EEPROM_BLOCK_TYPE_UNKNOWN (0)
#define EEPROM_BLOCK_TYPE_FREE (1)
#define EEPROM_BLOCK_TYPE_LISP (2)

#define MAGIC_BLOCK_ADDRESS (0)
#define MAGIC_WORD_ADDRESS (MAGIC_BLOCK_ADDRESS + sizeof(EEPROM_BLOCK))

typedef uint16_t eeprom_addr_t;

typedef struct eeprom_block {
    uint16_t type:2;
    uint16_t size:14;
} EEPROM_BLOCK;


#ifdef POSIX 
#include <fcntl.h>
#include <stdio.h>

/**
 * POSIX IMPLEMENTATION 
 */
eeprom_addr_t eeprom_get_start() {
    return 0;
}

eeprom_addr_t eeprom_get_end() {
    return 1024;
}

int eeprom_file_open() {
    int fd = open("eeprom.bin", O_RDWR);
    if (fd == -1) {
        if (errno == ENOENT) {
            // eeprom file does not exist, create and write freeblock to init
            char *nullchar[] = { '\0' };
            fd = open("eeprom.bin", O_CREAT | O_WRONLY);
            for (int i = eeprom_get_start(); i < eeprom_get_end(); i++) {
                write(fd, nullchar, 1);
            }
            close(fd);
            return eeprom_file_open();
        } else {
            perror("unable to open eeprom.bin");
            exit(-1);
        }
    } else {
        return fd;
    }
}

void eeprom_read(eeprom_addr_t addr, void *buff, size_t n) {
    int fd = eeprom_file_open();
    if ((lseek(fd, addr, SEEK_SET) != addr) || read(fd, buff, n) < n) {
        perror("unable to seek or read in eeprom");
        exit(-1);
    }
    close(fd);
}

uint8_t eeprom_read_byte(eeprom_addr_t addr) {
    uint8_t buff[1];
    eeprom_read(addr, buff, 1);
    return buff[0];
}

void eeprom_write(eeprom_addr_t addr, void *buff, size_t n) {
    int fd = eeprom_file_open();
    if ((lseek(fd, addr, SEEK_SET) != addr) || write(fd, buff, n) < n) {
        perror("unable to write in eeprom");
        exit(-1);
    }
    close(fd);
}

void eeprom_write_byte(eeprom_addr_t addr, uint8_t b) {
    eeprom_write(addr, &b, 1);
}

#else
/**
 * NON-POSIX
 */

size_t eeprom_get_start() {
    return 0;
}

size_t eeprom_get_end() {
    return EEPROM.length();
}

void eeprom_read(eeprom_addr_t addr, void *buff, size_t n) {
    while (n--) {
        EEPROM.read(addr, *(uint8_t*)buff);
        addr++;
        buff++;
    }
}

uint8_t eeprom_read_byte(eeprom_addr_t addr) {
    return EEPROM.read(addr);
}

void eeprom_write(eeprom_addr_t addr, void *buff, size_t n) {
    while (n--) {
        EEPROM.write(addr, *(uint8_t*)buff);
        addr++;
        buff++;
    }
}

void eeprom_write_byte(eeprom_addr_t addr, uint8_t b) {
    EEPROM.write(addr, b);
}

#endif


inline char* eeprom_addr_to_char_ptr(eeprom_addr_t addr) {
    return (char*)addr;
}

inline void* eeprom_addr_to_ptr(eeprom_addr_t addr) {
    return (void*)addr;
}

inline eeprom_addr_t eeprom_ptr_to_addr(void *ptr) {
    return (eeprom_addr_t)(ptr);
}

void eeprom_free(eeprom_addr_t addr) {
    size_t pos = 0;
    while (pos < eeprom_get_end()) {
        EEPROM_BLOCK fb;
        eeprom_read(pos, &fb, sizeof(fb));
        if (pos + sizeof(EEPROM_BLOCK) == addr) {
            fb.type = EEPROM_BLOCK_TYPE_FREE;
            eeprom_write(pos, &fb, sizeof(fb));
            pos += sizeof(fb);
            // zero emptied memory
            for (size_t pos_itr=pos; pos_itr < pos + fb.size; pos_itr++) {
                char *nullchar = { '\0' };
                eeprom_write(pos_itr, nullchar, 1);
            }
        } else {
            pos += fb.size + sizeof(fb);
        }
    }
}

eeprom_addr_t eeprom_alloc(size_t size, int type) {
    eeprom_addr_t pos = 0;
    while (pos < eeprom_get_end()) {
        EEPROM_BLOCK fb;
        eeprom_read(pos, &fb, sizeof(fb));
        if (fb.type == EEPROM_BLOCK_TYPE_FREE
                && fb.size >= size + sizeof(EEPROM_BLOCK)) {
            // confirm nothing has written to space
            size_t endpos = pos + size + sizeof(EEPROM_BLOCK) + sizeof(EEPROM_BLOCK);
            for (size_t pos_itr=pos; pos_itr < endpos; pos_itr++) {
                char c;
                eeprom_read(pos_itr, &c, 1);
                if (c != 0) {
                    return NOBLOCK_ADDR;
                }
            }
            fb.type = type;
            eeprom_write(pos, &fb, sizeof(EEPROM_BLOCK));
            fb.type = EEPROM_BLOCK_TYPE_FREE;
            fb.size = fb.size - size - sizeof(EEPROM_BLOCK);
            eeprom_write(pos + size + sizeof(EEPROM_BLOCK), &fb, sizeof(EEPROM_BLOCK));
            return pos + sizeof(EEPROM_BLOCK);
        } else {
            pos += fb.size + sizeof(EEPROM_BLOCK);
        }
    }
    return NOBLOCK_ADDR;
}


char eeprom_compare_eeprom(
        eeprom_addr_t eeprom_addr_a, 
        size_t a_len, 
        eeprom_addr_t eeprom_addr_b, 
        size_t b_len) {

    while (a_len && b_len) {
        uint8_t a_byte = eeprom_read_byte(eeprom_addr_a);
        uint8_t b_byte = eeprom_read_byte(eeprom_addr_b);
        if (a_byte > b_byte) {
            return 1;
        } else if (a_byte < b_byte) {
            return -1;
        }
        eeprom_addr_a++;
        a_len--;
        eeprom_addr_b++;
        b_len--;
    }
    if (a_len > 0) {
        return -1;
    } else if (b_len > 0) {
        return 1;
    } else {
        return 0;
    }
}

char eeprom_compare_pgm(
        eeprom_addr_t eeprom_addr_a, 
        size_t a_len, 
        PGM_P pgm_addr_b, 
        size_t b_len) {

    while (a_len && b_len) {
        uint8_t a_byte = eeprom_read_byte(eeprom_addr_a);
        uint8_t b_byte = PGM_READ_BYTE(pgm_addr_b);
        if (a_byte > b_byte) {
            return 1;
        } else if (a_byte < b_byte) {
            return -1;
        }
        eeprom_addr_a++;
        a_len--;
        pgm_addr_b++;
        b_len--;
    }

    if (a_len > 0) {
        return -1;
    } else if (b_len > 0) {
        return 1;
    } else {
        return 0;
    }
}

char eeprom_compare_ptr(
        eeprom_addr_t eeprom_addr_a, 
        size_t a_len, 
        char *b, 
        size_t b_len) {
            
    while (a_len && b_len) {
        uint8_t a_byte = eeprom_read_byte(eeprom_addr_a);
        if (a_byte > *b) {
            return 1;
        } else if (a_byte < *b) {
            return -1;
        }
        eeprom_addr_a++;
        a_len--;
        b++;
        b_len--;
    }

    if (a_len > 0) {
        return -1;
    } else if (b_len > 0) {
        return 1;
    } else {
        return 0;
    }
}


int eeprom_open() {
    const char * magic_word = PSTR("lisp");
    const size_t magic_word_size = 4;

    // check for magic word, if no magic word, initialize
    if (eeprom_compare_pgm(
            MAGIC_WORD_ADDRESS, 
            magic_word_size, 
            magic_word, 
            magic_word_size) != COMPARE_EQUAL) {
        eeprom_addr_t addr = MAGIC_BLOCK_ADDRESS;
        EEPROM_BLOCK block = {
            .type = EEPROM_BLOCK_TYPE_UNKNOWN,
            .size = magic_word_size,
        };

        // write block with magic word
        eeprom_write_byte(addr++, ((uint8_t*)&block)[0]);
        eeprom_write_byte(addr++, ((uint8_t*)&block)[1]);
        for (uint8_t i=0; i<magic_word_size; i++) {
            eeprom_write_byte(addr++, PGM_READ_BYTE(&magic_word[i]));
        }
        
        // write empty block
        block.type = EEPROM_BLOCK_TYPE_FREE;
        block.size = eeprom_get_end() - addr - sizeof(block);
        eeprom_write_byte(addr++, ((uint8_t*)&block)[0]);
        eeprom_write_byte(addr++, ((uint8_t*)&block)[1]);        
    }
    return TRUE;
}



#endif /* eeprom_h */
