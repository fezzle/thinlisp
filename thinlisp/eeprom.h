//
//  eeprom.h
//  thinlisp
//
//  Created by Eric on 2016-09-11.
//  Copyright © 2016 norg. All rights reserved.
//

#ifndef eeprom_h
#define eeprom_h

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "defines.h"
#include "runtime.h"


#define NOBLOCK_ADDR 0

#define EEPROM_LOCK_TYPE_UNKNOWN (0)
#define EEPROM_BLOCK_TYPE_LISP (1)


typedef uint16_t eeprom_addr_t;

typedef struct eeprom_block {
    uint16_t is_free:1;
    uint16_t type:2;
    uint16_t size:13;
} EEPROM_BLOCK;


#ifdef POSIX 
size_t eeprom_get_size() {
    return 1024;
}

int eeprom_open() {
    int fd = open("eeprom.bin", O_RDWR);
    if (fd == -1) {
        if (errno == ENOENT) {
            // eeprom file does not exist, create and write freeblock to init
            char *nullchar = { '\0' };
            fd = eeprom_open(O_CREAT | O_WRONLY);
            for (int i=0; i<eeprom_get_size(); i++) {
                write(fd, nullchar, 1);
            }
            close(fd);
            return eeprom_open();
        } else {
            perror("unable to open eeprom.bin");
            exit(-1);
        }
    } else {
        return fd;
    }
}


void eeprom_read(eeprom_addr_t addr, void *buff, size_t n) {
    int fd = eeprom_open();
    if ((lseek(fd, addr, SEEK_SET) != addr) || read(fd, buff, n) < n) {
        perror("unable to seek or read in eeprom");
        exit(-1);
    }
    close(fd);
}

void eeprom_write(eeprom_addr_t addr, void *buff, size_t n) {
    int fd = eeprom_open();
    if ((lseek(fd, addr, SEEK_SET) != addr) || write(fd, buff, n) < n) {
        perror("unable to write in eeprom");
        exit(-1);
    }
    close(fd);
}

void eeprom_free(eeprom_addr_t addr) {
    size_t end_of_eeprom = eeprom_get_size() - sizeof(EEPROM_BLOCK);
    size_t pos = 0;
    while (pos < end_of_eeprom) {
        EEPROM_BLOCK fb;
        eeprom_read(pos, &fb, sizeof(fb));
        if (pos + sizeof(EEPROM_BLOCK) == addr) {
            fb.is_free = 1;
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

eeprom_addr_t eeprom_alloc(size_t size) {
    eeprom_addr_t end_of_eeprom = eeprom_get_size() - sizeof(EEPROM_BLOCK);
    eeprom_addr_t pos = 0;
    while (pos < end_of_eeprom) {
        EEPROM_BLOCK fb;
        eeprom_read(pos, &fb, sizeof(fb));
        if (fb.is_free && fb.size >= size + sizeof(fb) + sizeof(fb)) {
            // confirm nothing has written to space
            size_t endpos = pos + size + sizeof(fb) + sizeof(fb);
            for (size_t pos_itr=pos; pos_itr < endpos; pos_itr++) {
                char c;
                eeprom_read(pos_itr, &c, 1);
                if (c != 0) {
                    return NOBLOCK_ADDR;
                }
            }
            fb.is_free = 0;
            eeprom_write(pos, &fb, sizeof(fb));
            fb.is_free = 1;
            fb.size = fb.size - size - sizeof(fb);
            eeprom_write(pos + size + sizeof(fb), &fb, sizeof(fb));
            return pos + sizeof(fb);
        } else {
            pos += fb.size + sizeof(fb);
        }
    }
    return NOBLOCK_ADDR;
}

#else

#define MAGIC_BLOCK_ADDRESS (0)
#define MAGIC_WORD_ADDRESS (MAGIC_BLOCK_ADDRESS + sizeof(EEPROM_BLOCK))
#define MAGIC_WORD_SIZE 4
#define MAGIC_WORD "lisp"

size_t eeprom_get_size() {
    return 1024//EEPROM.length();
}

bool eeprom_magic_word(eeprom_addr_t address, bool initialize) {
    /**
     * If `initialize` is true, the magic word is written to eeprom `address`.
     * If `initialize` is false, the magic word is compared to eeprom `address`
     *  with true being returned if equal.
     */
    char *magic_word = PSTR(MAGIC_WORD);
    for (char i = 0; i < MAGIC_WORD_SIZE; i++) {
        if (initialize) {
            EEPROM.write(address++, PGM_READ_BYTE(magic_word++));
        } else {
            if (EEPROM.read(address++) != PGM_READ_BYTE(magic_word++)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

void eeprom_initialize() {
    EEPROM_BLOCK block = {
        .is_free = 0,
        .type = EEPROM_BLOCK_TYPE_UNKNOWN,
        .size = 4,
    };
    EEPROM.write(MAGIC_BLOCK_ADDRESS, (uint8_t*)&block);
    EEPROM.write(MAGIC_BLOCK_ADDRESS + 1, ((uint8_t*)&block) + 1);
    eeprom_magic_word(MAGIC_WORD_ADDRSS, true));
}

int eeprom_open() {
    if (!eeprom_magic_word(MAGIC_WORD_ADDRESS, false)) {
        eeprom_initialize();
    }
}
#endif


#endif /* eeprom_h */
