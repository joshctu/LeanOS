#include <kernel/heap.h>
#include <kernel/vmm.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void heap_init(heap_t *heap) {
  heap_block_t dummy = {.next = 0, .size = 0, .sbsize = 0, .lfb = 0, .used = 0};
  heap->dummy = dummy;
}

int get_unique_writes(heap_block_t *b) {
  int ret = 0;
  uint8_t *bm = (uint8_t *)&b[1];
  for (uint32_t x = 0; x < (b->size / b->sbsize); x++) {
    if (bm[x] != 5 && bm[x] != 0) {
      ret++;
      int bid = bm[x];
      for (x; bm[x + 1] == bid; x++)
        ;
    }
  }
  return ret;
}

int heap_add_block(heap_t *heap, uintptr_t addr, uint32_t size, uint32_t sbsize) {
  heap_block_t *b;
  heap_block_t *last_block;
  uint32_t sbcnt;
  uint8_t *bm;

  b = (heap_block_t *)addr;
  b->size = size - sizeof(heap_block_t);
  b->sbsize = sbsize;
  b->next = 0;

  for (last_block = &heap->dummy; last_block->next; last_block = last_block->next)
    ;
  last_block->next = b;

  sbcnt = size / sbsize;

  bm = (uint8_t *)&b[1];

  // Clear bitmap
  for (size_t x = 0; x < sbcnt; x++) {
    bm[x] = 0;
  }

  // Now we need to calculate how many sub-blocks the bitmap takes up and mark
  // those sub-blocks as reserved. This is because each bm entry is 1 byte. Our
  // bitmap takes up x bytes where x is the number of sub-blocks.
  uint32_t bm_size = sbcnt;
  // We need some ceil division to account for partial sub-blocks
  bm_size = (bm_size + sbsize - 1) / sbsize;

  for (size_t x = 0; x < bm_size; x++) {
    bm[x] = 5;
  }

  b->lfb = sbcnt - 1;
  b->used = bm_size;

  return 1;
}

uint8_t gen_bm_id(uint8_t a, uint8_t b) {
  uint8_t c;
  for (c = a + 1; c == b || c == 0; c++)
    ;
  return c;
}

void *heap_malloc(heap_t *heap, uint32_t size) {
  heap_block_t *b;
  uint32_t sbcnt;
  uint32_t sbneed;
  uint32_t x, y, z;
  uint8_t *bm;
  uint32_t nid;

  for (b = heap->dummy.next; b; b = b->next) {

    if (b->size - (b->used * b->sbsize) >= size) {

      sbcnt = b->size / b->sbsize;
      sbneed = (size + b->sbsize - 1) / b->sbsize;

      bm = (uint8_t *)&b[1];

      for (x = (b->lfb + 1 >= sbcnt ? 0 : b->lfb + 1); x != b->lfb; x++) {
        if (x >= sbcnt) {
          x = 0;
        }

        if (bm[x] == 0) {

          for (y = 0; bm[x + y] == 0 && x + y < sbcnt && y < sbneed; y++)
            ;

          if (y == sbneed) {

            nid = gen_bm_id(bm[x - 1], bm[x + y]);

            for (z = 0; z < y; z++) {
              bm[x + z] = nid;
            }

            // This is the last allocated sub-block. We want to start here on
            // the next loop of this block.
            b->lfb = (x + sbneed) - 2;
            b->used += sbneed;

            return (void *)((x * b->sbsize) + (uintptr_t)&b[1]);
          }
        }
      }
    }
  }
  return 0;
}

int free(heap_t *heap, void *ptr) {
  heap_block_t *b;
  heap_block_t *prev;
  uint8_t *bm;
  size_t bm_idx, x;
  size_t bm_idx_id;

  for (prev = &heap->dummy; prev->next; prev = prev->next) {
    // Block has been found
    b = prev->next;
    if ((uintptr_t)ptr > (uintptr_t)b && (uintptr_t)ptr < (uintptr_t)&b[1] + b->size) {
      bm = (uint8_t *)&b[1];
      // Our address - the start of the data region gives relative address
      // into data region. Divide by sbsize to get start index.
      bm_idx = ((uintptr_t)ptr - (uintptr_t)&b[1]) / b->sbsize;
      bm_idx_id = bm[bm_idx];

      // Clear regions for writing
      for (x = bm_idx; bm[x] == bm_idx_id; x++) {
        bm[x] = 0;
      }

      b->lfb = bm_idx;
      b->used -= x - bm_idx;

      if (!get_unique_writes(b)) {
        prev->next = b->next;
        vmm_free_mem(b);
      }
      return 1;
    }
  }
  return 0;
}
