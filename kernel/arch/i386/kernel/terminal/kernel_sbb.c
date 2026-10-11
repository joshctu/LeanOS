#include <kernel/buffer_utils.h>
#include <kernel/kernel_sbb.h>
#include <kernel/shell.h>
#include <kernel/string_utils.h>
#include <kernel/tty.h>
#include <kernel/vga.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_SHELL_ARGS 32

static sbbuffer_instance_t kernel_sbb_instance;
static scrollback_buffer_t kernel_sbb = {.sbb_instance = &kernel_sbb_instance};

scrollback_buffer_t *get_shell_sbb() { return &kernel_sbb; }

/* Break up the current user entered data into sanitized tokens and send to the shell parser */
void tokenize_and_send_to_shell(char *input) {
  char *clean_input = trim_middling_whitespace(trim_trailing_whitespace(trim_prefixed_whitespace(input)));

  char *argv[MAX_SHELL_ARGS];
  size_t argc = 0;

  char *ptr = clean_input;
  char *start = clean_input;

  while (1) {

    if (argc >= MAX_SHELL_ARGS || strlen(start) == 0) {
      break;
    }

    if (*ptr == '\0') {
      argv[argc++] = start;
      break;
    }

    if (*ptr == ' ') {
      *ptr = '\0';
      argv[argc++] = start;
      start = ptr + 1;
    }
    ptr++;
  }

  argv[argc] = NULL;

  shell_read(argc, argv);
}

/* Reset the shell input buffer and rerender */
void input_proc_reset() {
  input_buf_reset();
  render_active_input_line();
}

/* Snap buffer to active line and call special terminal render method. This does not commit to sbb */
void render_active_input_line() {
  input_buf_t *input_buf = get_input_buf();
  shell_t *shell_info = get_shell_info();

  kernel_snap_to_active_line();
  terminal_render_line(&(tty_line_t){
      .prompt = shell_info->prompt,
      .buf = input_buf->data,
      .size = input_buf->size,
      .cursor = input_buf->cursor,
  });
}

/* Process key events from the terminal */
void shell_handle_key_event(key_event_t key_event) {

  input_buf_t *input_buf = get_input_buf();
  shell_t *shell_info = get_shell_info();

  switch (key_event.key) {
  case KEY_CHAR:
    if (key_event.ch == '\b') {
      input_buf_delete();
      render_active_input_line();
      return;
    }
    if (input_buf_write(key_event.ch)) {
      render_active_input_line();
    }
    break;
  case KEY_ENTER:
    kernel_sbb_write_line(&(tty_line_t){
        .prompt = shell_info->prompt,
        .buf = input_buf->data,
        .size = input_buf->size,
        .cursor = input_buf->cursor,
    });
    tokenize_and_send_to_shell(input_buf->data);
    input_proc_reset();
    break;
  }
}
void (*get_shell_key_handler(void))(key_event_t key_event) { return shell_handle_key_event; }

int get_kernel_sbb_head() { return kernel_sbb.sbb_head; }

// Initialize sbb set all entries blank
void kernel_sbb_init() { sbb_init(&kernel_sbb); }

int kernel_sbb_write_line(tty_line_t *line) {

  // TODO: I think this can just be render active input line
  terminal_render_line(line);

  line_t line_to_commit;

  for (size_t i = 0; i < VGA_WIDTH; i++) {
    line_to_commit[i] = vga_entry(' ', get_terminal_color());
  }

  terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
  for (int i = 0; line->prompt[i]; i++) {
    line_to_commit[i] = vga_entry(line->prompt[i], get_terminal_color());
  }

  terminal_setcolor(VGA_COLOR_GREEN);
  for (int i = 0; i < line->size; i++) {
    line_to_commit[strlen(line->prompt) + i] = vga_entry(line->buf[i], get_terminal_color());
  }

  sbb_write_line(&kernel_sbb, line_to_commit);
  return sbb_new_line(&kernel_sbb);
}

int kernel_shift_head(int shift_by, Direction dir) { return shift_head(&kernel_sbb, shift_by, dir); }

int kernel_snap_to_active_line() { return snap_to_active_line(&kernel_sbb); }
