#include <kernel/io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <kernel/buffer_utils.h>
#include <kernel/input_buf.h>
#include <kernel/input_proc.h>
#include <kernel/sbb.h>
#include <kernel/tty.h>
#include <kernel/vga.h>

static uint8_t terminal_color;
static uint16_t *terminal_buffer = (uint16_t *)0xC00B8000;

static int input_start;

static int sbb_row_idx;
static int sbb_col_idx;

static active_program_t active_program;

uint16_t *get_terminal_buffer() { return terminal_buffer; }
active_program_t *get_active_program() { return &active_program; }
size_t get_cursor(void) { return ((sbb_row_idx - get_sbb_head()) * VGA_WIDTH) + sbb_col_idx; }

void set_cursor(size_t pos) {
  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t)(pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t)((pos >> 8) & (0xFF)));
}

void terminal_initialize(void) {
  sbb_row_idx = 0;
  sbb_col_idx = 0;
  input_start = 0;
  terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
  sbb_init();
}

void cursor_right() {
  if (++sbb_col_idx == VGA_WIDTH) {
    sbb_col_idx = input_start;
    sbb_row_idx = increment_buf_value(sbb_row_idx, SBB_SIZE);

    if (sbb_row_idx > 24 && (sbb_row_idx - get_sbb_head()) % 25 == 0) {
      shift_head(1, DIR_DOWN);
    }
  }
}

void new_line() {

  // TODO: Trying to write to sbb line 222 causes a triple fault.
  if (++sbb_row_idx == 221) {
    return;
  };

  if (sbb_row_idx > 24 && (sbb_row_idx - get_sbb_head()) % 25 == 0) {
    shift_head(1, DIR_DOWN);
  }
  sbb_col_idx = input_start;
  set_cursor(get_cursor());
}

void write_fwd(char c) {
  sbb_write_char(sbb_row_idx, sbb_col_idx, vga_entry(c, terminal_color));
  cursor_right();
}

void terminal_setcolor(uint8_t color) { terminal_color = color; }

void terminal_render_line(const tty_line_t *line) {
  snap_to_active_line(sbb_row_idx);

  int curr_row = (sbb_row_idx - get_sbb_head() + SBB_SIZE) % SBB_SIZE;

  // Clear row
  for (size_t i = 0; i < VGA_WIDTH; i++) {
    terminal_buffer[(curr_row * VGA_WIDTH) + i] = vga_entry(' ', terminal_color);
  }

  sbb_col_idx = 0;

  // Draw prompt
  terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
  for (size_t i = 0; line->prompt[i]; i++) {
    terminal_buffer[(curr_row * VGA_WIDTH) + sbb_col_idx] = vga_entry(line->prompt[i], terminal_color);
    sbb_col_idx++;
  }

  // Draw buffer contents
  terminal_setcolor(VGA_COLOR_GREEN);
  for (size_t i = 0; i < line->size; i++) {
    terminal_buffer[(curr_row * VGA_WIDTH) + sbb_col_idx] = vga_entry(line->buf[i], terminal_color);
    sbb_col_idx++;
  }

  set_cursor(get_cursor());
}

void terminal_commit_line(const tty_line_t *line) {
  terminal_render_line(line);

  line_t line_to_commit;

  for (size_t i = 0; i < VGA_WIDTH; i++) {
    line_to_commit[i] = vga_entry(' ', terminal_color);
  }

  terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
  for (size_t i = 0; line->prompt[i]; i++) {
    line_to_commit[i] = vga_entry(line->prompt[i], terminal_color);
  }

  terminal_setcolor(VGA_COLOR_GREEN);
  for (size_t i = 0; i < line->size; i++) {
    line_to_commit[strlen(line->prompt) + i] = vga_entry(line->buf[i], terminal_color);
  }

  sbb_write_line(sbb_row_idx, &line_to_commit);
  new_line();
}

void terminal_write(const char *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (data[i] == '\n') {
      new_line();
    } else {
      write_fwd(data[i]);
    }
  }
}

void terminal_clear_screen() {
  sbb_reset();
  sbb_row_idx = 0;
  sbb_col_idx = 0;
  set_cursor(get_cursor());
}

bool handle_key_local(key_event_t event) {
  switch (event.key) {
  case KEY_PG_UP:
    shift_head(5, DIR_UP);
    set_cursor(get_cursor());
    return true;
    break;
  case KEY_PG_DOWN:
    shift_head(5, DIR_DOWN);
    set_cursor(get_cursor());
    return true;
    break;
  default:
    return false;
  }
}

void terminal_handle_key(key_event_t event) {

  if (handle_key_local(event))
    return;

  if (input_proc_handle_key(event))
    return;
}
