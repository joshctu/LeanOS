#include <kernel/buffer_utils.h>
#include <kernel/input_proc.h>
#include <kernel/sbb.h>
#include <kernel/tty.h>
#include <kernel/vga.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static scrollback_buffer_t sbb;
static int sbb_head = 0;
static int oldest_written;
static int newest_written;

int get_sbb_head() { return sbb_head; }

int get_oldest_written() { return oldest_written; }
int get_newest_written() { return newest_written; }
void set_oldest_written(int value) { oldest_written = value; }
void set_newest_written(int value) { newest_written = value; }

scrollback_buffer_t *get_sbb() { return &sbb; }

// Initialize sbb set all entries blank and sync to viewport
void sbb_init() {
  for (size_t line = 0; line < SBB_SIZE; line++) {
    for (size_t cell = 0; cell < 80; cell++) {
      sbb[line][cell] = vga_entry(' ', VGA_COLOR_GREEN);
    }
  }
  oldest_written = 0;
  newest_written = 0;
}

// Write to buffer and sync to vga
void sbb_write_char(int sbb_row_idx, int sbb_col_idx, uint16_t value) {
  if (sbb_row_idx != newest_written) {
    newest_written = sbb_row_idx;
    if (newest_written == oldest_written) {
      oldest_written = increment_buf_value(oldest_written, SBB_SIZE);
    }
  }

  sbb[sbb_row_idx][sbb_col_idx] = value;
}

void sbb_write_line(int sbb_row_idx, line_t *line) {
  if (sbb_row_idx != newest_written) {
    newest_written = sbb_row_idx;
    if (newest_written == oldest_written) {
      oldest_written = increment_buf_value(oldest_written, SBB_SIZE);
    }
  }

  memcpy(sbb[sbb_row_idx], *line, sizeof(*line));
}

void sbb_reset() {

  for (size_t line = 0; line < SBB_SIZE; line++) {
    for (size_t cell = 0; cell < 80; cell++) {
      sbb[line][cell] = vga_entry(' ', VGA_COLOR_GREEN);
    }
  }
  oldest_written = 0;
  newest_written = 0;
  sbb_head = 0;
}

bool is_at_bottom() { return (sbb_head == (newest_written - 23 + SBB_SIZE) % SBB_SIZE); }
bool is_at_top() { return sbb_head == 0; }
// Shift the sbb head pointer. For now, the logic to prevent the buffer
// from ever shifting past the oldest/newest written lines is baked in.
void shift_head(int shift_by, Direction dir) {
  if (dir == DIR_UP) {
    sbb_head = (sbb_head + SBB_SIZE - shift_by) % SBB_SIZE;
  } else {
    sbb_head = (sbb_head + shift_by) % SBB_SIZE;
  }

  int head_from_oldest = (sbb_head - oldest_written + SBB_SIZE) % SBB_SIZE;
  int head_from_newest = (((sbb_head + 24) % SBB_SIZE) - newest_written + SBB_SIZE) % SBB_SIZE;

  if (head_from_oldest > SBB_SIZE / 2)
    sbb_head = oldest_written;

  // This exists outside the normal flow becuase it needs to rerender the active
  // line if you page down to the end.
  if (head_from_newest < SBB_SIZE / 2)
    sbb_head = (newest_written - 23 + SBB_SIZE) % SBB_SIZE;
}

void snap_to_active_line(int sbb_row_idx) {
  if (sbb_row_idx < 24) {
    return;
  }

  while (sbb_row_idx - sbb_head != 24) {
    sbb_head = increment_buf_value(sbb_head, SBB_SIZE);
  }
}
