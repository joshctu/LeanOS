#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

typedef struct _heap_block_t {
  struct _heap_block_t *next;
  uint32_t size;
  uint32_t sbsize;
  uint32_t lfb;
  uint32_t used;
} heap_block_t;

typedef struct _heap_t {
  heap_block_t *fblock;
} heap_t;

void heap_init(heap_t *heap);
int heap_add_block(heap_t *heap, uintptr_t addr, uint32_t size, uint32_t sbsize);
void *heap_malloc(heap_t *heap, uint32_t size);
int free(heap_t *heap, void *ptr);

#endif
