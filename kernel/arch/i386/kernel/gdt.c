#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <kernel/gdt.h>

// Six entries in our table, null entry, code, data, user mode code, user mode
// data, and tss.
static uint8_t gdt_raw[GDT_SIZE] __attribute__((aligned(8)));

// Pointer holds the limit of the table and its starting address.
static gdt_ptr gdtr = {.limit = sizeof(gdt_raw) - 1,
                       .base = (uint32_t)&gdt_raw};

// Create TSS table
static tss_entry_t TSS = {0};

//Modify the neccesary tss entries.
void setup_tss(void) {
  TSS.esp0 = 0x0;
  TSS.ss0 = 0x10;
  TSS.iomap_base = sizeof(TSS);
}

uint16_t tss_selector = 0x28; //5th entry in 48 byte gdt (starts at byte 40).

/* Entries are code and data segments respectively. Flat mode so base and limit
 * are the same. Access byte denote code and data segments, present, ring 0
 * (kernel) executable and writable. 4KiB granularity for 4GiB memory space.*/
static gdt_entry gdt_entry_0 = {
    .base = 0x00000000, .limit = 0x00000000, .access_byte = 0x00, .flags = 0x0};

static gdt_entry gdt_entry_1 = {
    .base = 0x00000000, .limit = 0xFFFFF, .access_byte = 0x9A, .flags = 0xC};

static gdt_entry gdt_entry_2 = {
    .base = 0x00000000, .limit = 0xFFFFF, .access_byte = 0x92, .flags = 0xC};

static gdt_entry gdt_entry_3 = {
    .base = 0x00000000, .limit = 0xFFFFF, .access_byte = 0xFA, .flags = 0xC};

static gdt_entry gdt_entry_4 = {
    .base = 0x00000000, .limit = 0xFFFFF, .access_byte = 0xC, .flags = 0xC};

static gdt_entry gdt_entry_5 = {
    .base = (uint32_t)&TSS, .limit = sizeof(TSS) - 1, .access_byte = 0x89, .flags = 0x0};

/* Encode the table with each descriptor. A descriptor is an 8 byte value
 * that communicates the limit, base, access, and flags for types of data
 * access. */
void encodeGDTEntry(uint8_t *target, gdt_entry source) {
   if (source.limit > 0xFFFFF) 
	   printf("GDT cannot encode limits larger than 0xFFFFF");
  

  /* Encode the limit. Only take upper nibble for lower nibble of 6th byte
   * as its data is shared with flags. */
  target[0] = source.limit & 0xFF;
  target[1] = (source.limit >> 8) & 0xFF;
  target[6] = (source.limit >> 16) & 0x0F;

  // Encode the base.
  target[2] = source.base & 0xFF;
  target[3] = (source.base >> 8) & 0xFF;
  target[4] = (source.base >> 16) & 0xFF;
  target[7] = (source.base >> 24) & 0xFF;

  // Encode the access byte.
  target[5] = source.access_byte;

  // Encode the upper nibble for the 6th byte.
  target[6] |= (source.flags << 4);
}

/*
 * Creating a gdt with 5 entries. An entry per 8 bytes of the table  */
void createGDTEntries(uint8_t gdt_raw[GDT_SIZE]) {
  encodeGDTEntry(&gdt_raw[0], gdt_entry_0);
  encodeGDTEntry(&gdt_raw[8], gdt_entry_1);
  encodeGDTEntry(&gdt_raw[16], gdt_entry_2);
  encodeGDTEntry(&gdt_raw[24], gdt_entry_3);
  encodeGDTEntry(&gdt_raw[32], gdt_entry_4);
  encodeGDTEntry(&gdt_raw[40], gdt_entry_5);
}

/* Load the table into memory with inline assembly
 * volatile flag tells compiler not to optimze this is any way.
 * memory flag tells compiler this instruction alters memory so
 * it does not make any assumptions about the state of memory */
void gdt_flush(gdt_ptr gdtr) {
  __asm__ __volatile__("lgdt (%0)" : : "r"(&gdtr) : "memory");

  // Here we are reloading all the segement registers. We've defined where the
  // table lives, now we need to reload the registers to use the new table. So
  // we long jump to the code segment 0x08 (segment selector) with offset 1 (f
  // meaning forward, the next 1 in the code.). Then beginning at that 1, we run
  // the rest of the assembly, reloading our registers.
  __asm__ __volatile__(
      "ljmp $0x08, $1f\n\t"
      "1:\n\t"
      "mov $0x10, %%ax\n\t" // Here we move to the data segment 8 bits down from
                            // the code segment.
      "mov %%ax, %%ds\n\t"
      "mov %%ax, %%es\n\t"
      "mov %%ax, %%fs\n\t"
      "mov %%ax, %%gs\n\t"
      "mov %%ax, %%ss\n\t"
      :
      :
      : "memory", "ax" // register clobber
  );

  setup_tss();

  
//Load the task segment selector into the task register
  __asm__ __volatile__("ltr %0"
                       :
                       : "r"(tss_selector)
                       : "memory"

  );
}

/*
 * Entry into initializing the gdt */
void gdt_initialize(void) {

  createGDTEntries(gdt_raw);
  gdt_flush(gdtr);
}
