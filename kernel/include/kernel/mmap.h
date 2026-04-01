#ifndef _MMAP_H
#define _MMAP_H

#include <kernel/multiboot.h>
#include <stdint.h>

#define PMM_BITMAP_SIZE 32512
#define PMM_INVALID 0
uint32_t detect_available_mem(multiboot_info_t* mbi);
uint32_t get_kernel_size();
uint32_t alloc_phys_pg();
void dealloc_phys_pg();
void pmm_init(multiboot_info_t* mbi);

#endif
