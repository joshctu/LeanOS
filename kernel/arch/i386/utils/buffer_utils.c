#include <kernel/buffer_utils.h>
#include <kernel/vga.h>
#include <stdio.h>
#include <string.h>

int get_sbb_head(scrollback_buffer_t *sbb) { return sbb->sbb_head; }

int get_oldest_written_line(scrollback_buffer_t *sbb) { return sbb->oldest_written_line; }
int get_newest_written_line(scrollback_buffer_t *sbb) { return sbb->newest_written_line; }
void set_oldest_written_line(scrollback_buffer_t *sbb, int value) { sbb->oldest_written_line = value; }
void set_newest_written_line(scrollback_buffer_t *sbb, int value) { sbb->newest_written_line = value; }

int compute_screen_position(scrollback_buffer_t *sbb) {

  if (sbb->sbb_row_idx < sbb->sbb_head) {
    return (((sbb->sbb_row_idx + SBB_SIZE) - sbb->sbb_head) * VGA_WIDTH) + sbb->sbb_col_idx;
  } else {
    return ((sbb->sbb_row_idx - sbb->sbb_head) * VGA_WIDTH) + sbb->sbb_col_idx;
  }
}

void sbb_init(scrollback_buffer_t *sbb) {
  for (int line = 0; line < SBB_SIZE; ++line) {
    for (int cell = 0; cell < 80; ++cell) {
      (*sbb->sbb_instance)[line][cell] = vga_entry(' ', VGA_COLOR_GREEN);
    }
  }
  sbb->sbb_head = 0;
  sbb->input_start = 0;
  sbb->sbb_row_idx = 0;
  sbb->sbb_col_idx = 0;
  sbb->oldest_written_line = 0;
  sbb->newest_written_line = 0;
}

// Write to buffer. Note that value here is a vga entry. Not a raw char
void sbb_write_char(scrollback_buffer_t *sbb, uint16_t value) {
  if (sbb->sbb_row_idx != sbb->newest_written_line) {
    sbb->newest_written_line = sbb->sbb_row_idx;
    if (sbb->newest_written_line == sbb->oldest_written_line) {
      sbb->oldest_written_line = increment_buf_idx(sbb->oldest_written_line, SBB_SIZE);
    }
  }

  (*sbb->sbb_instance)[sbb->sbb_row_idx][sbb->sbb_col_idx] = value;
}

int sbb_cursor_right(scrollback_buffer_t *sbb) {
  if (++sbb->sbb_col_idx == VGA_WIDTH) {
    sbb->sbb_col_idx = sbb->input_start;

    sbb->sbb_row_idx = increment_buf_idx(sbb->sbb_row_idx, SBB_SIZE);
    if (sbb->sbb_row_idx > 24 && (sbb->sbb_row_idx - sbb->sbb_head) % 25 == 0) {
      shift_head(sbb, DIR_DOWN, 1);
    }
  }
  return compute_screen_position(sbb);
}

int sbb_new_line(scrollback_buffer_t *sbb) {
  // TODO: Trying to write to sbb line 222 causes a triple fault.
  if (++sbb->sbb_row_idx == 221) {
    return compute_screen_position(sbb);
  };

  if (sbb->sbb_row_idx > 24 && (sbb->sbb_row_idx - sbb->sbb_head) % 25 == 0) {
    shift_head(sbb, 1, DIR_DOWN);
  }
  sbb->sbb_col_idx = sbb->input_start;
  return compute_screen_position(sbb);
}

int sbb_write_line(scrollback_buffer_t *sbb, line_t *line) {
  if (sbb->sbb_row_idx != sbb->newest_written_line) {
    sbb->newest_written_line = sbb->sbb_row_idx;
    if (sbb->newest_written_line == sbb->oldest_written_line) {
      sbb->oldest_written_line = increment_buf_idx(sbb->oldest_written_line, SBB_SIZE);
    }
  }

  memcpy((*sbb->sbb_instance)[sbb->sbb_row_idx], *line, sizeof(*line));

  return compute_screen_position(sbb);
}

int sbb_reset(scrollback_buffer_t *sbb) {
  for (int line = 0; line < SBB_SIZE; line++) {
    for (int cell = 0; cell < 80; cell++) {
      (*sbb->sbb_instance)[line][cell] = vga_entry(' ', VGA_COLOR_GREEN);
    }
  }
  sbb->sbb_row_idx = 0;
  sbb->sbb_col_idx = 0;
  sbb->oldest_written_line = 0;
  sbb->newest_written_line = 0;
  sbb->sbb_head = 0;

  return compute_screen_position(sbb);
}

bool is_at_bottom(scrollback_buffer_t *sbb) { return (sbb->sbb_head == (sbb->newest_written_line - 23 + SBB_SIZE) % SBB_SIZE); }
bool is_at_top(scrollback_buffer_t *sbb) { return sbb->sbb_head == 0; }
// Shift the sbb head pointer. For now, the logic to prevent the buffer
// from ever shifting past the oldest/newest written lines is baked in.
int shift_head(scrollback_buffer_t *sbb, int shift_by, Direction dir) {
  if (dir == DIR_UP) {
    sbb->sbb_head = (sbb->sbb_head + SBB_SIZE - shift_by) % SBB_SIZE;
  } else {
    sbb->sbb_head = (sbb->sbb_head + shift_by) % SBB_SIZE;
  }

  int head_from_oldest = (sbb->sbb_head - sbb->oldest_written_line + SBB_SIZE) % SBB_SIZE;
  int head_from_newest = (((sbb->sbb_head + 24) % SBB_SIZE) - sbb->newest_written_line + SBB_SIZE) % SBB_SIZE;

  if (head_from_oldest > SBB_SIZE / 2)
    sbb->sbb_head = sbb->oldest_written_line;

  // This exists outside the normal flow becuase it needs to rerender the active
  // line if you page down to the end.
  if (head_from_newest < SBB_SIZE / 2)
    sbb->sbb_head = (sbb->newest_written_line - 23 + SBB_SIZE) % SBB_SIZE;

  return compute_screen_position(sbb);
}

/* Sets sbb head to 24 lines before given line number. Provided line number becomes last
visible line on the screen. */
int snap_to_active_line(scrollback_buffer_t *sbb) {

  while (sbb->sbb_row_idx - sbb->sbb_head > 24) {
    sbb->sbb_head = increment_buf_idx(sbb->sbb_head, SBB_SIZE);
  }

  return compute_screen_position(sbb);
}

void viewport_sync(scrollback_buffer_t *sbb, uint16_t *terminal_buffer) {
  line_t flat_range[25];
  for (int i = 0; i < 25; i++) {
    memcpy(&flat_range[i], &(*sbb->sbb_instance)[(i + sbb->sbb_head) % SBB_SIZE], sizeof(line_t));
  }
  memcpy(terminal_buffer, flat_range, sizeof(flat_range));
}

size_t get_buf_value(size_t value, size_t buf_size) { return (value % buf_size); }

size_t decrement_buf_value(size_t value, size_t buf_size) { return ((value + buf_size) - 1) % buf_size; }

size_t increment_buf_idx(size_t value, size_t buf_size) { return (value + 1) % buf_size; }
