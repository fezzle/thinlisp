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

#define MAX_SYMBOL_CHARS 32

#define NOBLOCK_ADDR 0


typedef struct eeprom_block {
    uint16_t is_free:1;
    uint16_t size;
} eeprom_block;

// values can be int, symbol, string, list
// int-12bits
typedef struct value {
    uint16_t type:2;
    union {
        uint16_t strlen:14;
        uint16_t listlen:14;
        uint16_t intval:14;
        uint16_t symbol:14;
    };
} value;

typedef struct symbol {
    uint16_t symbol;
    uint8_t strlen;
    char symbolstr[0];
} symbol;


size_t eeprom_get_size() {
    return 1024;
}


#ifdef ARDUINO

#else
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


void eeprom_read(size_t addr, void *buff, size_t n) {
    int fd = eeprom_open();
    if ((lseek(fd, addr, SEEK_SET) != addr) || read(fd, buff, n) < n) {
        perror("unable to seek or read in eeprom");
        exit(-1);
    }
    close(fd);
}

void eeprom_write(size_t addr, void *buff, size_t n) {
    int fd = eeprom_open();
    if ((lseek(fd, addr, SEEK_SET) != addr) || write(fd, buff, n) < n) {
        perror("unable to write in eeprom");
        exit(-1);
    }
    close(fd);
}

void eeprom_free(size_t addr) {
    size_t end_of_eeprom = eeprom_get_size() - sizeof(eeprom_block);
    size_t pos = 0;
    while (pos < end_of_eeprom) {
        eeprom_block fb;
        eeprom_read(pos, &fb, sizeof(fb));
        if (pos + sizeof(eeprom_block) == addr) {
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

size_t eeprom_alloc(size_t size) {
    size_t end_of_eeprom = eeprom_get_size() - sizeof(eeprom_block);
    size_t pos = 0;
    while (pos < end_of_eeprom) {
        eeprom_block fb;
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


typedef struct symboladdr {
    uint16_t symbol;
    uint16_t address;
} symboladdr;


size_t add_symbol(symbol s) {
    size_t addr = 0;
    symboladdr symaddr;
    while (1) {
        eeprom_read(addr, &symaddr, sizeof(symaddr));
        if (symaddr.symbol == s.symbol) {
            return symaddr.address;
        }
    }
            
    for (size_t pos=0; pos < eeprom_get_size();) {
        symbol symitr;
        if (lseek(fd, pos, SEEK_SET) == -1 ||
                read(fd, &symitr, sizeof(symbol)) == -1) {
            perror("unable to seek or read in eeprom.bin");
        }
        if (sym.symbol == symitr.symbol) {
            char symitr_str[MAX_SYMBOL_CHARS];
            read(fd, &symitr_str, sym.strlen);
            if (strncmp(symitr_str, sym.symbolstr, sym.strlen) == 0) {
                
                return pos;
            }
        } else {
            
        }
    }
    return 0;
}


#endif


#endif /* eeprom_h */
