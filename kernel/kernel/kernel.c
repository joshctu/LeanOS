#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/logs.h>
#include <kernel/mmap.h>
#include <kernel/multiboot.h>
#include <kernel/paging.h>
#include <kernel/pic.h>
#include <kernel/pit.h>
#include <kernel/shell.h>
#include <kernel/tty.h>
#include <kernel/vmm.h>

extern uint32_t _kernel_start;

void kernel_main(uint32_t magic, uint32_t addr) {
  multiboot_info_t *mbi;
  mbi = (multiboot_info_t *)(addr + 0xC0000000);

  gdt_initialize();
  PIC_remap();
  idt_initialize();

  terminal_initialize();
  pmm_init(mbi);

  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    printf("Invalid bootloader format\n");
  } else {
    printf("Booting using multiboot standard...\n");
  }

  generate_boot_logs(mbi);

  __asm__ __volatile__("sti");
  pit_reload(0xFFFE);

  vmm_reg_list_init();
  print_all_regions();
  char *new_string = vmm_kalloc_mem();
  strcpy(new_string, "This is a string that we are allocating at runtime");
  print_all_regions();
  vmm_free_mem(new_string);
  print_all_regions();

  // shell_init();
  while (1) {
    __asm__ __volatile__("hlt");
  }
};
