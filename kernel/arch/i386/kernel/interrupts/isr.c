#include <kernel/io.h>
#include <kernel/isr.h>
#include <kernel/pic.h>
#include <kernel/shell.h>
#include <kernel/tty.h>
#include <stdio.h>

char scancode_to_ascii[128] = {
    0,    '`', '1', '2',  '3', '4', '5', '6', '7',  '8', '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
    't',  'y', 'u', 'i',  'o', 'p', '[', ']', '\n', 0,   'a', 's', 'd', 'f', 'g',  'h',  'j', 'k', 'l', ';',
    '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v', 'b',  'n', 'm', ',', '.', '/', 0,    '*',  0,   ' ', 0,
};

char scancode_to_ascii_shifted[128] = {
    0,   '~', '!', '@', '#', '$', '%', '^', '&',  '*', '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R',
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,   'A', 'S', 'D', 'F', 'G',  'H',  'J', 'K', 'L', ':',
    '"', '~', 0,   '|', 'Z', 'X', 'C', 'V', 'B',  'N', 'M', '<', '>', '?', 0,    '*',  0,   ' ', 0,
};

// Interrupt attribute tells cpu to handle saving register state when this
// function is called, and restoring it when it returns. Must be passed the
// interrupt frame pointer.
__attribute__((interrupt, used)) void keyboard_interrupt_handler(interrupt_frame_t *f) {

  key_event_t new_event;

  uint8_t scancode = inb(0x60);

  static int shift_down = 0;
  static int extended_code = 0;

  // Handle extended byte codes
  if (scancode == 0xE0) {
    extended_code = 1;
    outb(PIC1_COMMAND, PIC_EOI);
  }

  if (extended_code) {
    // up arrow
    if (scancode == 0x48) {
      new_event.key = KEY_UP_ARROW;
      new_event.ch = '\0';
      terminal_handle_key(new_event);
      outb(PIC1_COMMAND, PIC_EOI);
      extended_code = 0;
      return;
    }
    // down arrow
    if (scancode == 0x50) {
      new_event.key = KEY_DOWN_ARROW;
      new_event.ch = '\0';
      terminal_handle_key(new_event);
      outb(PIC1_COMMAND, PIC_EOI);
      extended_code = 0;
      return;
    }
    // cursor left
    if (scancode == 0x4B) {
      new_event.key = KEY_LEFT_ARROW;
      new_event.ch = '\0';
      terminal_handle_key(new_event);
      outb(PIC1_COMMAND, PIC_EOI);
      extended_code = 0;
      return;
    }

    // cursor right
    if (scancode == 0x4D) {
      new_event.key = KEY_RIGHT_ARROW;
      new_event.ch = '\0';
      terminal_handle_key(new_event);
      outb(PIC1_COMMAND, PIC_EOI);
      extended_code = 0;
      return;
    }
    // page up
    if (scancode == 0x49) {
      new_event.key = KEY_PG_UP;
      new_event.ch = '\0';
      extended_code = 0;
      terminal_handle_key(new_event);
      outb(PIC1_COMMAND, PIC_EOI);
      return;
    }
    // page down
    if (scancode == 0x51) {
      new_event.key = KEY_PG_DOWN;
      new_event.ch = '\0';
      extended_code = 0;
      terminal_handle_key(new_event);
      outb(PIC1_COMMAND, PIC_EOI);
      return;
    }
  }

  // If shift release is detected
  if (scancode == 0xAA || scancode == 0xB6) {
    shift_down = 0;
    outb(PIC1_COMMAND, PIC_EOI);
    return;
  }

  // For now disable break codes.
  if (scancode >= 0x80) {
    outb(PIC1_COMMAND, PIC_EOI);
    return;
  }

  // If shift is pressed, set shift flag
  if (scancode == 0x2A || scancode == 0x36) {
    shift_down = 1;
    outb(PIC1_COMMAND, PIC_EOI);
    return;
  }

  // Enter key
  if (scancode == 0x1C) {
    new_event.ch = '\0';
    new_event.key = KEY_ENTER;
    terminal_handle_key(new_event);
    outb(PIC1_COMMAND, PIC_EOI);
    return;
  }

  // Backspace
  if (scancode == 0xE) {
    new_event.key = KEY_BACKSPC;
    new_event.ch = '\b';
    terminal_handle_key(new_event);
    outb(PIC1_COMMAND, PIC_EOI);
    return;
  }

  char c = shift_down ? scancode_to_ascii_shifted[scancode] : scancode_to_ascii[scancode];

  if (c) {
    new_event.ch = c;
    new_event.key = KEY_CHAR;
    terminal_handle_key(new_event);
  }

  outb(PIC1_COMMAND, PIC_EOI);
}

__attribute__((interrupt, used)) void timer_interrupt_handler(interrupt_frame_t *f) {
  static int tot_count = 0;

  if (tot_count == 100) {
    printf("\nTimer handler called\n");
    tot_count = 0;
    outb(PIC1_COMMAND, PIC_EOI);
    return;
  }

  tot_count++;
  outb(PIC1_COMMAND, PIC_EOI);
}

__attribute__((interrupt, used)) void page_fault_handler(interrupt_frame_t *f) {
  uint32_t fault_addr;
  asm volatile("mov %%cr2, %0" : "=r"(fault_addr));
  printf("page fault at: %x", fault_addr);
  asm volatile("cli; hlt");
}
