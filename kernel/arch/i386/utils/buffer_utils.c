#include <string.h>

size_t get_buf_value(size_t value, size_t buf_size) {
  return (value % buf_size);
}

size_t decrement_buf_value(size_t value, size_t buf_size) {
  return ((value + buf_size) - 1) % buf_size;
}

size_t increment_buf_value(size_t value, size_t buf_size) {
  return (value + 1) % buf_size;
}
