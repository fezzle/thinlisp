/*
** stolen from: http://rosettacode.org/wiki/VList
*/
#ifndef VLIST_H
#define VLIST_H

#include <stdio.h>
#include <stdlib.h>
#include "bistack.h"
#include "runtime.h"
#include "defines.h"

typedef struct sublist {
    struct sublist *next;
    void **elements;
} SUBLIST;

SUBLIST *sublist_new(BISTACK *bs, list_size_t s);

typedef struct vlist {
    SUBLIST *head;
    list_size_t last_size;
    list_size_t offset;
} VLIST;

VLIST *vlist_new(BISTACK *bs);

list_size_t vlist_size(VLIST *v);

void *vlist_element(VLIST *v, list_size_t idx);

list_size_t vlist_unshift(VLIST *v, BISTACK *bs, void *);

void *vlist_shift(VLIST *v);

#endif