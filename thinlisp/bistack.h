#ifndef BISTACK_H
#define BISTACK_H

#include <stddef.h>
#include <stdint.h>
#include <assert.h>


#include "runtime.h"

//bistack.c debug
//#define BS_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#define BS_DEBUG(...)

#define BS_FORWARD 0
#define BS_BACKWARD 1

#define SIZEOF_MARK (sizeof(void*))

typedef uint16_t bistack_lock_t;

static bistack_lock_t next_lock_id;

typedef struct bistack {
  void *forwardptr;
  void **forwardmark;

  void *backwardptr;
  void **backwardmark;

  uint8_t direction_stack;
  uint16_t forward_lock;
} BISTACK;

BISTACK *bistack_new(size_t size);
BISTACK *bistack_init(void *buffer, size_t len);
void bistack_destroy(BISTACK *bs);
int bistack_freemem(BISTACK *bs);

void *bistack_alloc(BISTACK *bs, uint16_t size);

void *bistack_allocf(BISTACK *bs, uint16_t size);

void *bistack_allocb(BISTACK *bs, uint16_t size);

void *bistack_mark(BISTACK *bs);

void *bistack_rewind(BISTACK *bs);

void *bistack_markf(BISTACK *bs);

void *bistack_rewindf(BISTACK *bs);

void *bistack_markb(BISTACK *bs);

void *bistack_rewindb(BISTACK *bs);

void bistack_zero(BISTACK *bs);

bistack_lock_t bistack_lockstack(BISTACK *bs) {
    lassert(bs->forward_lock == 0, BISTACK_LOCK_HELD);
    bs->forward_lock = ++next_lock_id;
    return bs->forward_lock;
}

void *bistack_reservestack(BISTACK *bs, bistack_lock_t lock, size_t size) {
    /**
     * Reserves a desired number of bytes on the stack
     * :param lock: a lock provided by a call to bistack_lockstack.
     * :param size: the number of bytes to be reserved.
     */
    lassert(bs->forward_lock == lock, BISTACK_LOCK_HELD);
    bistack_markf(bs);
    return bistack_allocf(bs, size);
}

void *bistack_claimstack(BISTACK *bs, bistack_lock_t lock, size_t size) {
    /**
     * Claims a number of bytes from a reserved stack.  The previous bistack
     * call must have been a bistack_reservestack call.
     */
    lassert(bs->forward_lock == lock, BISTACK_LOCK_HELD);

    // rewind mark, set mark again and allocate desired size
    bistack_rewindf(bs);
    bistack_markf(bs);
    return bistack_allocf(bs, size);
}

void *bistack_rewindstack(BISTACK *bs) {
    return bistack_rewindf(bs);
}

void *bistack_pushstack(BISTACK *bs, bistack_lock_t lock, size_t size) {
    lassert(bs->forward_lock == lock, BISTACK_LOCK_HELD);
    return bistack_allocf(bs, size);
}

void bistack_unlock(BISTACK *bs, bistack_lock_t lock) {
    lassert(bs->forward_lock == lock, BISTACK_LOCK_HELD);
    bs->forward_lock = 0;
}


void *bistack_dropmark(BISTACK *bs);
void *bistack_dropmarkf(BISTACK *bs);
void *bistack_dropmarkb(BISTACK *bs);

inline void *bistack_push(BISTACK *bs, uint16_t size) {
    return bistack_allocf(bs, size);
}

inline void *bistack_heapalloc(BISTACK *bs, uint16_t size) {
    return bistack_allocb(bs, size);
}

inline void *bistack_heapmark(BISTACK *bs) {
    return bistack_markb(bs);
}

inline void *bistack_heaprewind(BISTACK *bs) {
    return bistack_rewindb(bs);
}

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
