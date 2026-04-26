#include <kernel/input_buf.h>
#include <kernel/shell.h>
#include <kernel/tty.h>

#include <stdbool.h>
#include <stdio.h>

#define MAX_SHELL_ARGS 32

void render_line() {
  active_program_t *active_program = get_active_program();
  input_buf_t *input_buf = get_input_buf();

  terminal_render_line(&(tty_line_t){
      .prompt = active_program->shell->prompt,
      .buf = input_buf->data,
      .size = input_buf->size,
      .cursor = input_buf->cursor,
  });
}

void input_proc_reset() {
  input_buf_reset();
  render_line();
}

void input_proc_sanitize_buf() {}

char *trim_prefixed_whitespace(char *input) {
  char *ptr = input;

  while (*ptr == ' ') {
    ptr++;
  }
return ptr;
}

char *trim_trailing_whitespace(char *input) {
  size_t len = strlen(input);

  if (len == 0)
    return input;

  char *ptr = input + len - 1;

  while (*ptr == ' ') {
    ptr--;
  }

  *(ptr + 1) = '\0';
  return input;
}

char *trim_middling_whitespace(char *input) {
  size_t len = strlen(input);

  if (len == 0)
    return input;

  char *ptr_read = input;
  char *ptr_write = input;

  while (*ptr_read != '\0') {
    if (*ptr_read == ' ') {
      *ptr_write = *ptr_read;
      ptr_read++;
      ptr_write++;
      while (*ptr_read == ' ') {
        ptr_read++;
      }
    }
    *ptr_write = *ptr_read;
    ptr_write++;
    ptr_read++;
  }
  *ptr_write = '\0';
  return input;
}

void tokenize_and_send_to_shell(char *input) {
  char *clean_input = trim_middling_whitespace(
      trim_trailing_whitespace(trim_prefixed_whitespace(input)));

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

// There may come a day where this needs to handle more than the shell.
bool input_proc_handle_key(key_event_t event) {
  input_buf_t *input_buf = get_input_buf();
  active_program_t *active_program = get_active_program();

  switch (event.key) {
  case KEY_CHAR:
    if (event.ch == '\b') {
      input_buf_delete();
      render_line();
      return true;
    }
    if (input_buf_write(event.ch)) {
      render_line();
      return true;
    }
    break;
  case KEY_ENTER:
    terminal_commit_line(&(tty_line_t){
        .prompt = active_program->shell->prompt,
        .buf = input_buf->data,
        .size = input_buf->size,
        .cursor = input_buf->cursor,
    });

    tokenize_and_send_to_shell(input_buf->data);
    input_proc_reset();
    return true;
    break;
  default:
    return false;
  }
  return false;
}
