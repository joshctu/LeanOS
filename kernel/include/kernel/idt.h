#ifndef _IDT_H_
#define _IDT_H_

#include <stdint.h>

#define IDT_ENTRY_COUNT 256
#define IDT_SIZE (IDT_ENTRY_COUNT * 8)

typedef struct {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t zero;
	uint8_t type_attributes;
	uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idt_ptr_t;

void idt_initialize(void);



#endif

