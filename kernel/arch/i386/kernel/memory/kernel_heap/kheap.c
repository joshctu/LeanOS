#include <kernel/heap.h>
#include <kernel/kheap.h>
#include <kernel/vmm.h>
#include <stdint.h>
#include <stdio.h>

static heap_t kheap;

void kheap_init() { heap_init(&kheap); }

void *kmalloc(uint32_t size) {
  if (kheap.fblock == 0) {
    heap_add_block(&kheap, (uintptr_t)vmm_alloc_pages(VMM_REGION_KERNEL, 1, 0x003), 0x0001000, 16);
  }

  void *addr = heap_malloc(&kheap, size);

  if ((uintptr_t)addr == 0) {
    heap_add_block(&kheap, (uintptr_t)vmm_alloc_pages(VMM_REGION_KERNEL, 1, 0x003), 0x0001000, 16);
    addr = heap_malloc(&kheap, size);
  }

  return addr;
}

int kfree(void *ptr) {
  int status = free(&kheap, ptr);
  return status;
}

void kheap_reg_dump() {
  heap_block_t *b;
  int block = 1;
  uint8_t *bm;

  for (b = kheap.fblock; b; b = b->next) {
    int unique_writes = 0;
    bm = (uint8_t *)&b[1];

    for (uint32_t x = 0; x < (b->size / b->sbsize); x++) {
      if (bm[x] != 5 && bm[x] != 0) {
        unique_writes++;
        int bid = bm[x];
        for (x; bm[x + 1] == bid; x++)
          ;
      }
    }
    printf("Block number: %d\n", block);
    printf("    Bytes allocated from block: %d/%d\n", b->used * b->sbsize, b->size);
    printf("    Unique writes: %d\n", unique_writes);
    printf("    Subblock allocation: %d/%d\n", b->used, b->size / b->sbsize);
    block++;
  }
}
