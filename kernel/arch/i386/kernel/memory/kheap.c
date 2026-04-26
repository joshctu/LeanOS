#include <kernel/kheap.h>

#include <stddef.h>
#include <stdint.h>

kheap_t kheap;

void kheap_init(kheap_t *kheap) { kheap->fblock = 0; }

int kheap_add_block(kheap_t *kheap, uintptr_t addr, uint32_t size, uint32_t sbsize) {
  kheap_block_t *b;
  uint32_t sbcnt;
  uint8_t *bm;

  b = (kheap_block_t *)addr;
  b->size = size - sizeof(kheap_block_t);
  b->sbsize = sbsize;

  b->next = kheap->fblock;
  kheap->fblock = b;

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
  b->used = 0;

  return 1;
}

uint8_t gen_bm_id(uint8_t a, uint8_t b) {
  uint8_t c;
  for (c = a + 1; c == b || c == 0; c++)
    ;
  return c;
}

void *kheap_malloc(kheap_t *kheap, uint32_t size) {
  kheap_block_t *b;
  uint32_t sbcnt;
  uint32_t sbneed;
  uint32_t x, y, z;
  uint8_t *bm;
  uint32_t nid;

  for (b = kheap->fblock; b; b = b->next) {

    if (b->size - (b->used * b->sbsize) >= size) {

      sbcnt = b->size / b->sbsize;

      sbneed = (size + b->sbsize - 1) / b->sbsize;

      bm = (uint8_t *)&b[1];

      for (x = (b->lfb + 1 >= sbcnt ? 0 : b->lfb); x < b->lfb; x++) {
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

int kheap_free(kheap_t *kheap, void *ptr) {
  kheap_block_t *b;
  uint8_t *bm;
  size_t bm_idx, x;
  size_t bm_idx_id;

  for (b = kheap->fblock; b; b = b->next) {

    // Block has been found
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
      b->used -= (x - 1);

      return 1;
    }
  }
  return 0;
}
