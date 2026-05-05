#include <kernel/mmap.h>
#include <kernel/vmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

vm_mem_map_t vm_map;

static uint32_t *kpd_addr = (uint32_t *)0xFFFFF000;
static vm_reg_t mem_reg_nodes[MAX_VM_REGIONS];
static vm_reg_t *free_list = NULL;

void free_list_init() {
  for (uint32_t i = 0; i < MAX_VM_REGIONS; i++) {
    mem_reg_nodes[i].next = free_list;
    free_list = &mem_reg_nodes[i];
  }
}

vm_reg_t *alloc_reg() {

  if (!free_list)
    return NULL;

  vm_reg_t *r = free_list;
  free_list = r->next;

  return r;
}

void free_reg(vm_reg_t *r) {
  r->next = free_list;
  free_list = r;
}
void add_reg_to_map(vm_reg_t vm_reg, uint32_t flags) {
  vm_reg_t *node = alloc_reg();

  *node = vm_reg;
  node->flags = flags;
  vm_map.curr_reg->next = node;
  vm_map.curr_reg = node;
}

// Bootstrap the region list
void vmm_reg_list_init() {
  free_list_init();
  vm_reg_t *reserved_region = alloc_reg();
  reserved_region->start_addr = 0x00000000;
  reserved_region->end_addr = 0x00100000;
  vm_reg_t *dummy_reg = alloc_reg();
  dummy_reg->next = reserved_region;

  vm_map.dummy_reg = dummy_reg;
  vm_map.curr_reg = reserved_region;

  vm_reg_t reg;
  for (uint32_t pde = 0; pde < 1024; pde++) {

    // Page table not set
    if (!(kpd_addr[pde] & 1))
      continue;

    // Special case: recursive mapping
    if (pde == 1023) {
      reg.start_addr = 0xFFFFF000;
      reg.end_addr = 0xFFFFFFFF;
      add_reg_to_map(reg, 0x003);
      continue;
    }
    // Use recursive mapping to get pt ptr
    uint32_t *pt = (uint32_t *)(0xFFC00000 + pde * 0x1000);

    bool in_region = false;
    uint32_t flags;
    for (uint32_t pf = 0; pf < 1024; pf++) {

      uint32_t pte = pt[pf];
      uint32_t virt = (pde << 22) | (pf << 12);

      if (pte & 1) {

        // Region start
        if (!in_region) {
          reg.start_addr = virt;
          flags = pte & 0xFFF;
          in_region = true;
        }

      } else {
        // Region end
        if (in_region || (pte & 0xFFF) != flags) {
          reg.end_addr = virt;
          add_reg_to_map(reg, flags);
          in_region = false;
        }
      }
    }

    // 4MB region end
    if (in_region) {
      reg.end_addr = (pde << 22) + (4 * 1024 * 1024);
      add_reg_to_map(reg, flags);
    }
  }
}

vm_reg_t *get_start_of_kernel_reg() {
  vm_reg_t *i;
  for (i = vm_map.dummy_reg->next; i->start_addr != 0xC0000000 && i; i = i->next)
    ;

  return i;
}

// This currently will only allocate continuous free regions.
// Probably want this to be able to allocate n free regions regardless of continuity
void *vmm_alloc_pages(vmm_region_t vmm_region, size_t n, uint32_t flags) {

  vm_reg_t *reg_start_node;
  uint32_t reg_end_addr;
  uint32_t reg_flags = flags;

  switch (vmm_region) {
  case VMM_REGION_KERNEL:
    reg_start_node = get_start_of_kernel_reg();
    reg_end_addr = 0xFFFFFFFF;
    break;

  case VMM_REGION_USER:
    reg_start_node = vm_map.dummy_reg->next;
    reg_end_addr = 0xC0000000;
    reg_flags |= 0x4;
    break;

  default:
    return NULL;
  }

  uintptr_t last_mapped_va;

  for (vm_reg_t *curr_region = reg_start_node; curr_region && curr_region->end_addr <= reg_end_addr; curr_region = curr_region->next) {
    last_mapped_va = curr_region->end_addr;

    if (!curr_region->next)
      break;

    // Is this current free region big enough for the requested number of pages.
    if (curr_region->next->start_addr - last_mapped_va >= 0x1000 * n) {
      uint32_t pd_index = (last_mapped_va >> 22) & 0x3FF;

      if (!(kpd_addr[pd_index] & 0x1)) {
        kpd_addr[pd_index] = alloc_phys_pg() | reg_flags;
      }

      uint32_t pt_index = (last_mapped_va >> 12) & 0x3FF;
      uint32_t *pg_table = (uint32_t *)(0xFFC00000 + pd_index * 0x1000);

      // Allocate and insert n pages since we found a valid location
      for (size_t pages = 0; pages < n; pages++) {
        pg_table[pt_index + pages] = alloc_phys_pg() | reg_flags;
        vm_reg_t *new_reg = alloc_reg();
        uintptr_t start_addr = last_mapped_va + (0x1000 * pages);
        new_reg->start_addr = start_addr;
        new_reg->end_addr = start_addr + 0x1000;
        new_reg->flags = pg_table[pt_index] & 0xFFF;

        new_reg->next = curr_region->next;
        curr_region->next = new_reg;

        curr_region = curr_region->next;
      }
      return (void *)(last_mapped_va);
    }
  }
  return NULL;
}

// HELPER FXN DELETE LATER
void print_all_regions() {

  for (vm_reg_t *i = vm_map.dummy_reg->next; i; i = i->next) {
    printf("%x -> %x Flags: %x\n", i->start_addr, i->end_addr, i->flags);
  }
}

int vmm_free_mem(void *ptr) {

  for (vm_reg_t *i = vm_map.dummy_reg; i->next->next; i = i->next) {
    // This loop strategy exists so we can maintain the reference to the previous node for removal.
    if (i->next->start_addr == (uint32_t)ptr) {
      uint32_t pde = ((uint32_t)ptr >> 22) & 0x3FF;
      uint32_t *pte = (uint32_t *)(0xFFC00000 + pde * 0x1000);
      uint32_t pt = ((uint32_t)ptr >> 12) & 0x3FF;
      uint32_t x;

      // Zero out 4Kb page
      dealloc_phys_pg(pte[pt] & 0xFFFFF000);
      pte[pt] = 0;
      __asm__ __volatile__("invlpg (%0)" ::"r"(i->next->start_addr) : "memory");

      for (x = 0; pte[x] == 0 && x < 1024; x++)
        ;

      if (pte[x] == 0) {
        dealloc_phys_pg(kpd_addr[pde] & 0xFFFFF000);
        kpd_addr[pde] = 0;
      }

      vm_reg_t *old_reg = i->next;
      i->next = old_reg->next;

      free_reg(old_reg);
      return 1;
    }
  }
  return 0;
};
