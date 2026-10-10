#ifndef INPUT_BUF_H
#define INPUT_BUF_H

#include <stdbool.h>
#include <stdint.h>

#define INPUT_BUF_SIZE 66

typedef struct {
  char data[INPUT_BUF_SIZE];
  uint8_t cursor;
  uint8_t size;
} input_buf_t;

input_buf_t *get_input_buf();
bool input_buf_write(char c);
void input_buf_delete();
void input_buf_reset();

#endif
