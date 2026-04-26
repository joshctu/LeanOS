#include <kernel/mmap.h>
#include <kernel/multiboot.h>
#include <kernel/paging.h>
#include <stdint.h>
#include <stdio.h>

extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

// 4 kB frames
static uint8_t pmm[PMM_BITMAP_SIZE];

uint32_t detect_available_mem(multiboot_info_t *mbi) {
  uint32_t available_ram = 0;

  multiboot_memory_map_t *start;

  start = (multiboot_memory_map_t *)(mbi->mmap_addr + 0xC0000000);

  for (; (uintptr_t)start < (mbi->mmap_addr + 0xC0000000) + mbi->mmap_length;
       start = (multiboot_memory_map_t *)((uintptr_t)start + start->size + sizeof(start->size))) {
    if (start->type == 1) {

      available_ram += start->len;
    }
  }

  return available_ram;
}

// Return kernel size in bytes
uint32_t get_kernel_size() { return (((uint32_t)&_kernel_end - 0xC0000000) - (uint32_t)&_kernel_start); }

uint32_t align_addr_up(uint32_t addr) {
  do {
    addr += 1;
  } while (addr % 4096 != 0);

  return addr;
}

uint32_t align_addr_down(uint32_t addr) {
  do {
    addr -= 1;
  } while (addr % 4096 != 0);

  return addr;
}

void reserve_kernel_space() {
  // Kernel size in bytes divided by 4096 bytes in a frame gives us frames
  int frames = (int)(get_kernel_size() / 4096) + 1;

  // Loaded in at 1MB divided by the bytes per page gives us starting page
  int starting_kernel_frame = 0x00100000 / 4096;
  int final_frame = starting_kernel_frame + frames;
  //  printf("%d\n", final_frame);
  // We want to reserve the first MB as well to not run the risk of overwriting bios info
  for (int i = 0; i < final_frame; i++) {
    pmm[i] = 1;
  }
}

void pmm_init(multiboot_info_t *mbi) {
  for (int i = 0; i < PMM_BITMAP_SIZE; i++) {
    pmm[i] = 1;
  }

  multiboot_memory_map_t *start = (multiboot_memory_map_t *)(mbi->mmap_addr + 0xC0000000);

  for (; (uintptr_t)start < (mbi->mmap_addr + 0xC0000000) + mbi->mmap_length;
       start = (multiboot_memory_map_t *)((uintptr_t)start + (start->size + sizeof(start->size)))) {

    if (start->type == 1) {
      uint32_t start_addr = align_addr_up(start->addr);
      uint32_t end_addr = align_addr_down((start->addr + start->len));

      int frame_start = start_addr / 4096;
      int frame_end = end_addr / 4096;

      for (int i = frame_start; i < frame_end; i++) {
        pmm[i] = 0;
      }
    }
  }

  reserve_kernel_space();
}

uint32_t alloc_phys_pg() {

  for (uint32_t i = 1; i < PMM_BITMAP_SIZE; i++) {

    if (pmm[i] == 0) {
      pmm[i] = 1;
      return i * 4096;
    }
  }

  return PMM_INVALID;
}

void dealloc_phys_pg(uint32_t frame_addr) {

  int frame = frame_addr / 4096;
  pmm[frame] = 0;
}
