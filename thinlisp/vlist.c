/*
** stolen from: http://rosettacode.org/wiki/VList
*/

#include <stdio.h>
#include <stdlib.h>
#include "bistack.h"
#include "runtime.h"
#include "defines.h"
#include "vlist.h"

SUBLIST *sublist_new(BISTACK *bs, list_size_t s) {
    SUBLIST *sub = bistack_alloc(bs, sizeof(SUBLIST) + sizeof(void *) * s);
    sub->elements = (void **)(sub + 1);
    sub->next = NULL;
    return sub;
}

VLIST *vlist_new(BISTACK *bs) {
    VLIST *v = bistack_alloc(bs, sizeof(VLIST));
    v->head = sublist_new(bs, 1);
    v->last_size = 1;
    v->offset = 0;
    return v;
}

list_size_t vlist_size(VLIST *v) {
    return v->last_size * 2 - v->offset - 2;
}

void **vlist_address(VLIST *v, list_size_t idx) {
    SUBLIST *s = v->head;
    list_size_t top = v->last_size;
    list_size_t i = idx + v->offset;

    if (i + 2 >= (top << 1)) {
        lerror(VLIST_INDEX_ERROR, PSTR("vlist address out of range"));
    }
    while (s && i >= top) {
        s = s->next;
        i ^= top;
        top >>= 1;
    }
    return s->elements + i;
}

void *vlist_element(VLIST *v, list_size_t idx) {
    return *vlist_address(v, idx);
}

list_size_t vlist_unshift(VLIST *v, BISTACK *bs, void *x) {
    SUBLIST *s;
    void **p;

    if (!v->offset) {
        s = sublist_new(bs, v->last_size << 1);
        v->offset = (v->last_size <<= 1);
        s->next = v->head;
        v->head = s;
    }
    *(p = v->head->elements + --v->offset) = x;
    return p;
}

void *vlist_shift(VLIST *v) {
    SUBLIST *s;
    void *x;

    if (v->last_size == 1 && v->offset == 1) {
        lerror(VLIST_SHIFT_ON_EMPTY, PSTR("vlist_shift called on empty array"));
    }
    x = v->head->elements[v->offset++];

    if (v->offset == v->last_size) {
        v->offset = 0;
        if (v->last_size > 1) {
            s = v->head, v->head = s->next;
            v->last_size >>= 1;
            free(s);
        }
    }
    return x;
}

#ifdef VLIST_TEST
int main() {
    int i;

    BISTACK *bs = bistack_new(10000);

    VLIST *v = vlist_new(bs);
    for (i = 0; i < 10; i++) {
        vlist_unshift(v, bs, i);
    }

    printf("size: %d\n", vlist_size(v));
    for (i = 0; i < 10; i++)
        printf("v[%d] = %d\n", i, vlist_element(v, i));
    for (i = 0; i < 10; i++)
        printf("shift: %d\n", vlist_shift(v));

    /* v_shift(v); */ /* <- boom */

    //vlist_delete(v);
    return 0;
}
#endif