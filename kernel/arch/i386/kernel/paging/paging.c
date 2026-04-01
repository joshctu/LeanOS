#include <kernel/mmap.h>
#include <kernel/paging.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

page_directory_t pg_directory;

void paging_enable() {
  uint32_t cr0;
  asm volatile("mov %%cr0, %0" : "=r"(cr0));
  cr0 |= 0x80000000;
  asm volatile("mov %0, %%cr0\n"
               "jmp 1f\n"
               "1:\n" ::"r"(cr0));
};

void load_pg_directory(uint32_t *pd) {
  asm volatile("mov %0, %%cr3" ::"r"(pd));
};

void paging_init(void) {

  static page_table_t kernel_page_table;
  static page_table_t identity_page_table;
  printf("kernel end: %x\n", (uint32_t)&_kernel_end);
  for (int i = 0; i < 1024; i++) {
    kernel_page_table[i] = (0x100000 + i * 0x1000) | 0x3;
    identity_page_table[i] = (i * 0x1000) | 0x3;
  }

  for (int i = 0; i < 1024; i++) {
    pg_directory[i] = 0;
  }

  pg_directory[0] = ((uint32_t)identity_page_table | 0x3);
  pg_directory[768] = ((uint32_t)kernel_page_table | 0x3);

  load_pg_directory(pg_directory);
  paging_enable();
};

/*void map_page(uint32_t virtual_addr, uint32_t phy_addr, uint32_t
*pg_directory, uint32_t flags) { uint32_t pd_index = virtual_addr >> 22;
  uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

  uint32_t *pt;

  if (pg_directory[pd_index] & 0x1) {
    pt = (uint32_t *)(pg_directory[pd_index] & ~0xFFF);
  } else {
    uint32_t new_phys_pg = alloc_phys_pg();
    pt = (uint32_t *)new_phys_pg;

    memset(pt, 0, 4096);

    pg_directory[pd_index] = new_phys_pg | flags;
  }

  pt[pt_index] = phy_addr | flags;
}

void map_higher_half_kernel() {
  for (uint32_t phys = (uint32_t)&_kernel_start; phys < (uint32_t)&_kernel_end;
       phys += 4096) {
    uint32_t virt = phys + 0xC0000000;

    map_page(virt, phys, pg_directory, 0x3);
  }
}*/
