#include <kernel/heap.h>
#include <kernel/kheap.h>
#include <kernel/vmm.h>
#include <stdint.h>
#include <stdio.h>

static heap_t kheap;

void kheap_init() { heap_init(&kheap); }

void *kmalloc(uint32_t size) {
  // Check 4KB block amount needed to accomodate this allocation
  int blk_requirement = (int)(size / 3800) + 1;
  // If no block on heap
  if (kheap.dummy.next == 0) {
    heap_add_block(&kheap, (uintptr_t)vmm_alloc_pages(VMM_REGION_KERNEL, 1 * blk_requirement, 0x003), 0x0001000 * blk_requirement, 16);
  }

  // First malloc attempt
  void *addr = heap_malloc(&kheap, size);

  // If failed add new block and try again
  if ((uintptr_t)addr == 0) {
    heap_add_block(&kheap, (uintptr_t)vmm_alloc_pages(VMM_REGION_KERNEL, 1 * blk_requirement, 0x003), 0x0001000 * blk_requirement, 16);
    addr = heap_malloc(&kheap, size);

    if (addr == 0) {
      return NULL;
    }
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

  for (b = kheap.dummy.next; b; b = b->next) {
    int unique_writes = get_unique_writes(b);
    printf("Block number: %d\n", block);
    printf("    Bytes allocated from block: %d/%d\n", b->used * b->sbsize, b->size);
    printf("    Unique writes: %d\n", unique_writes);
    printf("    Subblock allocation: %d/%d\n", b->used, b->size / b->sbsize);
    block++;
  }
}
