#ifndef _ISR_H_
#define _ISR_H_

#include <stdint.h>

typedef struct interrupt_frame {
  uint32_t eip;
  uint32_t cs;
  uint32_t eflags;
  uint32_t esp;
  uint32_t ss;
} interrupt_frame_t;

__attribute__((interrupt, used)) void
keyboard_interrupt_handler(interrupt_frame_t *f);
__attribute__((interrupt, used)) void page_fault_handler(interrupt_frame_t *f);
__attribute__((interrupt, used)) void
timer_interrupt_handler(interrupt_frame_t *f);

#endif
