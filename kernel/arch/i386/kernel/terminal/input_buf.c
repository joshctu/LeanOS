#include <kernel/buffer_utils.h>
#include <kernel/input_buf.h>

#include <stdbool.h>
#include <stdint.h>

static input_buf_t input_buf = {
    .cursor = 0,
    .size = 0,
};

input_buf_t *get_input_buf() { return &input_buf; }

bool input_buf_write(char c) {
  if (input_buf.size >= INPUT_BUF_SIZE - 2) {
    return false;
  }

  input_buf.data[input_buf.cursor] = c;

  input_buf.cursor++;
  input_buf.size++;

  return true;
}

void input_buf_delete() {
  if (input_buf.size == 0) {
    return;
  }

  input_buf.cursor--;
  input_buf.data[input_buf.cursor] = '\0';
  input_buf.size--;
}

void input_buf_reset() {

  for (int i = 0; i < input_buf.size; i++) {
    input_buf.data[i] = '\0';
  }
  input_buf.cursor = 0;
  input_buf.size = 0;
}
