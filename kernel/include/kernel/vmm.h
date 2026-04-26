#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define MAX_VM_REGIONS 4096

typedef struct _vm_reg_t {
  struct _vm_reg_t *next;
  uintptr_t start_addr;
  uintptr_t end_addr;
  uint32_t flags;
} vm_reg_t;

typedef struct _vm_mem_map_t {
  vm_reg_t *dummy_reg;
  vm_reg_t *curr_reg;
} vm_mem_map_t;

void vmm_reg_list_init();
void *vmm_kalloc_mem();
int vmm_free_mem(void *ptr);
void print_all_regions();

#endif
