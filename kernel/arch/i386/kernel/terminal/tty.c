#include <kernel/io.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <kernel/buffer_utils.h>
#include <kernel/input_buf.h>
#include <kernel/kernel_sbb.h>
#include <kernel/tty.h>
#include <kernel/vga.h>

static uint8_t terminal_color;
static uint16_t *terminal_buffer = (uint16_t *)0xC00B8000;

static int tty_row_idx;
static int tty_col_idx;

tty_owner_t tty_owner;
uint16_t *get_terminal_buffer() { return terminal_buffer; }
uint8_t get_terminal_color() { return terminal_color; }

void set_tty_owner(scrollback_buffer_t *sbb, void (*handler_fxn)(key_event_t key_event)) {
  tty_owner.handler_fxn = handler_fxn;
  tty_owner.context = sbb;
}

void terminal_set_cursor(size_t pos) {
  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t)(pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t)((pos >> 8) & (0xFF)));
  tty_row_idx = pos / VGA_WIDTH;
  tty_col_idx = pos % VGA_WIDTH;
}

void terminal_initialize(void) {
  terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
  // Default owner is shell
  sbb_init(get_shell_sbb());
  tty_owner.handler_fxn = get_shell_key_handler();
  tty_owner.context = get_shell_sbb();
  viewport_sync(tty_owner.context);
}

void cursor_right() {
  int cursor_pos = sbb_cursor_right(tty_owner.context);
  terminal_set_cursor(cursor_pos);
}

void new_line() {
  int cursor_pos = sbb_new_line(tty_owner.context);
  terminal_set_cursor(cursor_pos);
}

void write_fwd(char c) {
  sbb_write_char(tty_owner.context, vga_entry(c, terminal_color));
  int cursor_pos = sbb_cursor_right(tty_owner.context);
  viewport_sync(tty_owner.context);
  terminal_set_cursor(cursor_pos);
}

void viewport_sync(scrollback_buffer_t *sbb) {
  line_t flat_range[25];
  for (int i = 0; i < 25; i++) {
    memcpy(&flat_range[i], &(*sbb->sbb_instance)[(i + sbb->sbb_head) % SBB_SIZE], sizeof(line_t));
  }
  memcpy(terminal_buffer, flat_range, sizeof(flat_range));
}

void terminal_setcolor(uint8_t color) { terminal_color = color; }

void terminal_render_line(const tty_line_t *line) {

  // Set the sbb header to correct value
  int cursor_pos = snap_to_active_line(tty_owner.context);
  viewport_sync(tty_owner.context);

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
  terminal_set_cursor(cursor_pos);
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
  int cursor_pos = sbb_reset(tty_owner.context);
  viewport_sync(tty_owner.context);
  terminal_set_cursor(cursor_pos);
}

bool handle_key_local(key_event_t event) {
  switch (event.key) {
  case KEY_PG_UP:
    if (is_at_top(tty_owner.context)) {
      return true;
    }
    int cursor_pos = shift_head(tty_owner.context, 5, DIR_UP);
    viewport_sync(tty_owner.context);
    terminal_set_cursor(cursor_pos);
    return true;
    break;
  case KEY_PG_DOWN:
    shift_head(tty_owner.context, 5, DIR_DOWN);
    viewport_sync(tty_owner.context);
    if (is_at_bottom(tty_owner.context)) {
      // render_active_input_line();
    }
    return true;
    break;
  default:
    return false;
  }
}

void terminal_handle_key(key_event_t event) {

  tty_owner.handler_fxn(event);
  return;
}
