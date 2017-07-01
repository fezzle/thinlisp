#ifndef BISTACK_H
#define BISTACK_H

#include <stdint.h>
#include <assert.h>
#include "heap.h"

#define BS_FORWARD 0
#define BS_BACKWARD 1

typedef struct bistack {
  void *forwardptr;
  MARK *forwardmark;

  MARK *backwardmark;
  void *backwardptr;

  uint8_t direction_stack;
} BISTACK;

BISTACK *bistack_new(size_t size);
BISTACK *bistack_init(void *buffer, size_t len);
void bistack_destroy(BISTACK *bs);
int bistack_freemem(BISTACK *bs);

void *bistack_alloc(BISTACK *bs, uint16_t size);

void *bistack_allocf(BISTACK *bs, uint16_t size);

void *bistack_allocb(BISTACK *bs, uint16_t size);

void bistack_mark(BISTACK *bs);

void bistack_rewind(BISTACK *bs);

void bistack_markf(BISTACK *bs);

void bistack_rewindf(BISTACK *bs);

void bistack_markb(BISTACK *bs);

void bistack_rewindb(BISTACK *bs);

void bistack_dropmark(BISTACK *bs);
void bistack_dropmarkf(BISTACK *bs);
void bistack_dropmarkb(BISTACK *bs);

static inline void bistack_pushdir(BISTACK *bs, unsigned char dir) {
  assert((bs->direction_stack & 0x80) == 0);
  bs->direction_stack <<= 1;
  bs->direction_stack |= dir;
}

static inline void bistack_popdir(BISTACK *bs) {
  bs->direction_stack >>= 1;
}

static inline uint8_t bistack_dir(BISTACK *bs) {
  return bs->direction_stack & 1;
}

#endif
