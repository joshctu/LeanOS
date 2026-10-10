#include <kernel/io.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <kernel/buffer_utils.h>
#include <kernel/input_buf.h>
#include <kernel/input_proc.h>
#include <kernel/kernel_sbb.h>
#include <kernel/tty.h>
#include <kernel/vga.h>

static uint8_t terminal_color;
static uint16_t *terminal_buffer = (uint16_t *)0xC00B8000;

static int tty_row_idx;
static int tty_col_idx;

uint16_t *get_terminal_buffer() { return terminal_buffer; }

void set_cursor(size_t pos) {
  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t)(pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t)((pos >> 8) & (0xFF)));
  tty_row_idx = pos / VGA_WIDTH;
  tty_col_idx = pos % VGA_WIDTH;
}

void terminal_initialize(void) {
  terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
  kernel_sbb_init();
  kernel_viewport_sync(terminal_buffer);
}

void cursor_right() {
  int cursor_pos = kernel_sbb_cursor_right();
  kernel_viewport_sync(terminal_buffer);
  set_cursor(cursor_pos);
}

void new_line() {
  int cursor_pos = kernel_sbb_new_line();
  kernel_viewport_sync(terminal_buffer);
  set_cursor(cursor_pos);
}

void write_fwd(char c) {
  kernel_sbb_write_char(vga_entry(c, terminal_color));
  int cursor_pos = kernel_sbb_cursor_right();
  kernel_viewport_sync(terminal_buffer);
  set_cursor(cursor_pos);
}

void terminal_setcolor(uint8_t color) { terminal_color = color; }

void terminal_render_line(const tty_line_t *line) {

  // Set the sbb header to correct value
  int cursor_pos = kernel_snap_to_active_line();
  kernel_viewport_sync(terminal_buffer);

  // Draw prompt
  terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
  int i = 0;
  for (; line->prompt[i]; i++) {
    terminal_buffer[cursor_pos + i] = vga_entry(line->prompt[i], terminal_color);
  }

  // Draw buffer contents
  terminal_setcolor(VGA_COLOR_GREEN);
  for (int j = 0; j < line->size; j++) {
    terminal_buffer[cursor_pos + i] = vga_entry(line->buf[j], terminal_color);
    i++;
  }

  cursor_pos += i;
  set_cursor(cursor_pos);
}

void terminal_commit_line(const tty_line_t *line) {
  terminal_render_line(line);

  line_t line_to_commit;

  for (size_t i = 0; i < VGA_WIDTH; i++) {
    line_to_commit[i] = vga_entry(' ', terminal_color);
  }

  terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
  for (int i = 0; line->prompt[i]; i++) {
    line_to_commit[i] = vga_entry(line->prompt[i], terminal_color);
  }

  terminal_setcolor(VGA_COLOR_GREEN);
  for (int i = 0; i < line->size; i++) {
    line_to_commit[strlen(line->prompt) + i] = vga_entry(line->buf[i], terminal_color);
  }

  kernel_sbb_write_line(&line_to_commit);
  int cursor_pos = kernel_sbb_new_line();
  kernel_viewport_sync(terminal_buffer);
  set_cursor(cursor_pos);
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
  int cursor_pos = kernel_sbb_reset();
  kernel_viewport_sync(terminal_buffer);
  set_cursor(cursor_pos);
}

bool handle_key_local(key_event_t event) {
  switch (event.key) {
  case KEY_PG_UP:
    if (kernel_sbb_is_at_top()) {
      return true;
    }
    int cursor_pos = kernel_shift_head(5, DIR_UP);
    kernel_viewport_sync(terminal_buffer);
    set_cursor(cursor_pos);
    return true;
    break;
  case KEY_PG_DOWN:
    kernel_shift_head(5, DIR_DOWN);
    kernel_viewport_sync(terminal_buffer);
    if (kernel_sbb_is_at_bottom()) {
      render_active_input_line();
    }
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
