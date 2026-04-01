#include <kernel/outbuf.h>
#include <kernel/shell.h>
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <string.h>

static outbuf_t buf = {.head = 0, .tail = 0, .size = 0};
static int cursor_in_input = 0;

// ALL KEYBOARD INTERRUPTS COME THROUGH THIS BUFFER FIRST
char buffer_read() {

  if (buf.size == 0) {

    return '\0';
  }

  char tail_char = buf.data[buf.tail++];
  buf.size--;
  return tail_char;
}

void buffer_write(char c) {
  char *prompt = get_prompt();
  size_t prompt_len = strlen(prompt);

  buf.data[buf.head] = c;
  buf.head = buf.head + 1 % OUTPUT_BUFFER_SIZE;

  if (buf.head == buf.tail) {
    buf.tail = buf.head + 1 % OUTPUT_BUFFER_SIZE;
  }

  if (buf.size != OUTPUT_BUFFER_SIZE - 1) {
    buf.size++;
  }

  // This is where we determine whether in interrupt should be written to the
  // shell buffer. If the current cursor pos >=
  while (buf.size != 0) {
    char next_char = buffer_read();
    if (next_char == '\n') {
      new_line();
    } else if (next_char == '\b') {
      // This guards against backspacing past the prompt
      if (--cursor_in_input < 0) {
        cursor_in_input++;
        return;
      }
      stdin_buffer_bck();
      write_bck();
    } else {
      // For now we guard against any writes that would exceed the line limit
      // TODO: Add write in place logic here. Function already exists.
      if (++cursor_in_input > (VGA_WIDTH - 1 - prompt_len)) {
        cursor_in_input--;
        return;
      }
      // Shell buffer
      stdin_buffer_write(next_char);
      // Terminal buffer
      write_fwd(next_char);
    }
  }
}

void handle_shell_submit() {
  shell_read();
  cursor_in_input = 0;
}

void guard_cursor_left() {
  if (--cursor_in_input < 0) {
    cursor_in_input++;
    return;
  }
  cursor_left();
}

void guard_cursor_right() {
  char *prompt = get_prompt();
  size_t prompt_len = strlen(prompt);
  if (++cursor_in_input > (VGA_WIDTH - 1 - prompt_len)) {
    cursor_in_input--;
    return;
  }
  cursor_right();
}
