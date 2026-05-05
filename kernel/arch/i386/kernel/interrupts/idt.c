#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <kernel/idt.h>
#include <kernel/isr.h>

static idt_entry_t idt_raw[IDT_ENTRY_COUNT] __attribute__((aligned(8)));

static idt_ptr_t idtr = {.limit = sizeof(idt_raw) - 1, .base = (uint32_t)&idt_raw};

idt_entry_t format_kernel_level_idt_handler(uint32_t handler_address) {
  idt_entry_t idt_entry = {.offset_low = (uint16_t)(handler_address & 0xFFFF),
                           .selector = 0x08,
                           .zero = 0,
                           .type_attributes = 0x8E,
                           .offset_high = (uint16_t)((handler_address >> 16) & 0xFFFF)};

  return idt_entry;
}

void createIDTEntries(void) {
  //  idt_raw[13] = format_kernel_level_idt_handler((uint32_t)&primary_drive_handler);
  idt_raw[14] = format_kernel_level_idt_handler((uint32_t)&page_fault_handler);
  // Timer interrupt
  idt_raw[32] = format_kernel_level_idt_handler((uint32_t)&timer_interrupt_handler);
  // Keyboard interrupt
  idt_raw[33] = format_kernel_level_idt_handler((uint32_t)&keyboard_interrupt_handler);
}

void idt_flush(void) { __asm__ __volatile__("lidt (%0)" : : "r"(&idtr) : "memory"); }

void idt_initialize(void) {
  createIDTEntries();
  idt_flush();
}
