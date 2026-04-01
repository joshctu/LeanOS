#include <kernel/sbb.h>
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

scrollback_buffer_t sbb;
uint16_t *terminal_buffer = (uint16_t *)0xC03FF000;

static int sbb_head = 0;

void viewport_sync(uint16_t *terminal_buffer) {
  line_t flat_range[25];
  for (int i = 0; i < 25; i++) {
    memcpy(flat_range[i], sbb[(i + sbb_head) % 1000], sizeof(line_t));
  }
  memcpy(terminal_buffer, flat_range, 4000);
}

void sbb_init() {
  for (size_t line = 0; line < SBB_SIZE; line++) {
    for (size_t cell = 0; cell < 80; cell++) {
      sbb[line][cell] = vga_entry(' ', VGA_COLOR_GREEN);
    }
  }
  viewport_sync(terminal_buffer);
}

void sbb_write(size_t cursor_pos, uint16_t value) {

  int cell_idx = cursor_pos % 80;
  int line_idx = ((cursor_pos / 80) + sbb_head) % 1000;

  sbb[line_idx][cell_idx] = value;
  viewport_sync(terminal_buffer);
}

void shift_head(size_t cursor_pos) {
  if (cursor_pos < 80) {
    sbb_head = (sbb_head + SBB_SIZE - 1) % SBB_SIZE;
  } else {
    sbb_head = (sbb_head + 1) % SBB_SIZE;
  }

  viewport_sync(terminal_buffer);
}
