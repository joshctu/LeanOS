#ifndef _GDT_H_
#define _GDT_H_

#include <stdint.h>

#define GDT_ENTRY_COUNT 6
#define GDT_SIZE (GDT_ENTRY_COUNT * 8)

typedef struct {
	uint32_t base;
	uint32_t limit;
	uint8_t access_byte;
	uint8_t flags;
} __attribute__((packed)) gdt_entry;

typedef struct {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) gdt_ptr;

typedef struct {
	uint32_t link; //used to link back to previous tss when hardware switching. Not used for us.
	uint32_t esp0; //Kernel stack pointer for ring 0. Points to top of stack
	uint32_t ss0;  //Kernel data segment selector for ring 0. Kernel stack is located here.
	uint32_t esp1; //Kernel stack pointer for ring 1. Not used here.
	uint32_t ss1;  //Kernel data segment selector for ring 1. Not used here.
	uint32_t esp2; //Kernel stack pointer for ring 2. Not used here.
	uint32_t ss2;  //Kernel data segment selector for ring 2. Not used here.
	uint32_t cr3;  //If using hardware switching this is the physical memory address of the process directory.
		       //Not used here.
	uint32_t eip;  //Extended instruction pointer, tells cpu where to pick up execution.
	uint32_t eflags; //Gives additional info about operations. Not set here in hardware switching.
	uint32_t eax, ecx, edx, ebx; //Stores the values of these registers for reloading on process start.
	uint32_t esp, ebp, esi, edi; //General purpose stack registers. Notably esp and ebp. ebp is base pointer
				     //that is generated per function call. It provides a stable reference point
				     //for function params, local variables, etc, for that function to be referenced.
	uint32_t es, cs, ss, ds, fs, gs; //Reloads which segment the cpu was operating in.
	uint32_t ldt; //Local descriptor table address. Not used in flat mode.
	uint16_t iomap_base; //Bitmap that lets the cpu know which ports this proccess has access to.
} __attribute__((packed, aligned(8))) tss_entry_t;

void gdt_initialize(void);
void createGDTEntries(uint8_t gdt_raw[GDT_SIZE]);
void encodeGDTEntry(uint8_t *target, gdt_entry source);
extern void gdt_flush(gdt_ptr gdtr);

#endif
