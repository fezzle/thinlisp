#ifndef HEAP_H
#define HEAP_H

typedef struct mark {
  void *ptr;
  struct mark *next;
} MARK;


typedef struct heap {
  void *start;  
  void *ptr;
  void *end;
  MARK *lastmark;
} HEAP;


void heap_mark(HEAP *heap);
void heap_rewind(HEAP *heap);
void *heap_alloc(HEAP *heap, int size);
HEAP *heap_new(int size);

inline int heap_available(HEAP *heap) {
  return heap->end - heap->ptr;
}

inline void *heap_curptr(HEAP *heap) {
  return heap->ptr;
}


#endif
