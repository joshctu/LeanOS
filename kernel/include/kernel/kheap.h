#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>
#include <stdint.h>

typedef struct _kheap_block_t {
  struct _kheap_block_t *next;
  uint32_t size;
  uint32_t sbsize;
  uint32_t lfb;
  uint32_t used;
} kheap_block_t;

typedef struct _kheap_t {
  kheap_block_t *fblock;
} kheap_t;

int kheap_add_block(kheap_t *kheap, uintptr_t addr, uint32_t size,
                    uint32_t sbsize);
void *kheap_malloc(kheap_t *kheap, uint32_t size);
int kheap_free(kheap_t *kheap, void *ptr);

#endif
