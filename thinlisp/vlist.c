/*
** stolen from: http://rosettacode.org/wiki/VList
*/

#include <stdio.h>
#include <stdlib.h>
#include "bistack.h"
#include "runtime.h"

typedef struct sublist {
  struct sublist* next;
  int *buf;
} SUBLIST;
 
SUBLIST *sublist_new(BISTACK *bs, size_t s) {
  SUBLIST* sub = bistack_alloc(bs, sizeof(SUBLIST) + sizeof(int) * s);
  sub->buf = (int*)(sub + 1);
  sub->next = 0;
  return sub;
}
 
typedef struct vlist {
	SUBLIST* head;
	size_t last_size, ofs;
} VLIST;
 
VLIST *vlist_new(BISTACK *bs) {
  VLIST *v = bistack_alloc(bs, sizeof(VLIST));
  v->head = sublist_new(bs, 1);
  v->last_size = 1;
  v->ofs = 0;
  return v;
}
 
inline size_t vlist_size(VLIST *v) {
  return v->last_size * 2 - v->ofs - 2;
}
 
int* vlist_address(VLIST *v, size_t idx) {
  SUBLIST *s = v->head;
  size_t top = v->last_size, i = idx + v->ofs;
 
  if (i + 2 >= (top << 1)) {
    lerror(VLIST_INDEX_ERROR, PSTR("vlist address out of range"));
  }
  while (s && i >= top) {
    s = s->next;
    i ^= top;
    top >>= 1;
  }
  return s->buf + i;
}
 
inline int vlist_element(VLIST *v, size_t idx) {
  return *vlist_address(v, idx);
}
 
int* vlist_unshift(VLIST *v, BISTACK *bs, int x) {
  SUBLIST* s;
  int *p;
 
  if (!v->ofs) {
    s = sublist_new(bs, v->last_size << 1);
    v->ofs = (v->last_size <<= 1);
    s->next = v->head;
    v->head = s;
  }
  *(p = v->head->buf + --v->ofs) = x;
  return p;
}
 
int vlist_shift(VLIST *v) {
  SUBLIST* s;
  int x;
 
  if (v->last_size == 1 && v->ofs == 1) {
    lerror(VLIST_SHIFT_ON_EMPTY, PSTR("vlist_shift called on empty array"));
  }
  x = v->head->buf[v->ofs++];
 
  if (v->ofs == v->last_size) {
    v->ofs = 0;
    if (v->last_size > 1) {
      s = v->head, v->head = s->next;
      v->last_size >>= 1;
      free(s);
    }
  }
  return x;
}
 
int main() {
  int i;
 
  VLIST *v = vlist_new();
  for (i = 0; i < 10; i++) v_unshift(v, i);
 
  printf("size: %d\n", v_size(v));
  for (i = 0; i < 10; i++) printf("v[%d] = %d\n", i, v_elem(v, i));
  for (i = 0; i < 10; i++) printf("shift: %d\n", v_shift(v));
 
  /* v_shift(v); */ /* <- boom */
 
  v_del(v);
  return 0;
}
