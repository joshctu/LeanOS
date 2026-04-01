#include <kernel/io.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/sbb.h>
#include <kernel/tty.h>
#include <kernel/vga.h>

static int terminal_row;
static int terminal_column;
static int input_start;
static uint8_t terminal_color;

static stdin_buffer_t stdin_buffer = {.head = 0, .tail = 0, .size = 0};

stdin_buffer_t *get_stdin_buffer() { return &stdin_buffer; }

void stdin_buffer_write(char c) {
  stdin_buffer.data[stdin_buffer.head] = c;
  stdin_buffer.head = (stdin_buffer.head + 1) % SHELL_BUF_SIZE;

  if (stdin_buffer.head == stdin_buffer.tail) {
    stdin_buffer.tail = (stdin_buffer.head + 1) % SHELL_BUF_SIZE;
  }
  if (stdin_buffer.size != SHELL_BUF_SIZE) {
    stdin_buffer.size++;
  }
}

void stdin_buffer_bck() {
  if (stdin_buffer.size == 0) {
    return;
  }

  stdin_buffer.head = stdin_buffer.head - 1 % SHELL_BUF_SIZE;
  stdin_buffer.size--;
}

char stdin_buffer_read() {
  if (stdin_buffer.size == 0) {
    return '\0';
  }

  char tail_char = stdin_buffer.data[stdin_buffer.tail];
  stdin_buffer.tail = stdin_buffer.tail + 1 % SHELL_BUF_SIZE;
  stdin_buffer.size--;
  return tail_char;
}

size_t get_cursor(void) { return terminal_row * VGA_WIDTH + terminal_column; }

void set_cursor(void) {

  if (terminal_column < input_start) {
    terminal_column = input_start;
  }

  size_t pos = get_cursor();

  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t)(pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t)((pos >> 8) & (0xFF)));
}

void set_input_start(int idx) {
  input_start = idx;
  set_cursor();
}

void terminal_initialize(void) {
  terminal_row = 0;
  input_start = 0;
  terminal_column = input_start;
  terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
  sbb_init();
}

void cursor_right() {
  if (++terminal_column == VGA_WIDTH) {
    terminal_column = VGA_WIDTH - 1;
  }
  set_cursor();
}

void cursor_left() {
  if (--terminal_column < input_start) {
    terminal_column = input_start;
  }
  set_cursor();
}

void cursor_up() {
  size_t pos = get_cursor();
  if (--terminal_row < 0) {
    shift_head(pos);
    terminal_row++;
  }
  set_cursor();
}

void cursor_down() {
  size_t pos = get_cursor();
  if (++terminal_row > VGA_HEIGHT) {
    shift_head(pos);
    terminal_row--;
  }
  set_cursor();
}

void new_line() {
  size_t pos = get_cursor();
  terminal_column = input_start;
  if (++terminal_row == VGA_HEIGHT) {
    shift_head(pos);
    terminal_row--;
  }
  set_cursor();
}

void write_in_place(char c) {
  sbb_write(get_cursor(), vga_entry(c, terminal_color));
}

void write_fwd(char c) {
  sbb_write(get_cursor(), vga_entry(c, terminal_color));
  cursor_right();
}

void write_bck() {
  cursor_left();
  sbb_write(get_cursor(), vga_entry(' ', terminal_color));
}

void terminal_setcolor(uint8_t color) { terminal_color = color; }

void terminal_write(const char *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (data[i] == '\n') {
      new_line();
    } else if (data[i] == '\b') {
      write_bck();
    }

    else {
      write_fwd(data[i]);
    }
  }
}
void terminal_writestring(const char *data) {
  terminal_write(data, strlen(data));
}
