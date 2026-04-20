#ifndef BUFFER_UTILS_H
#define BUFFER_UTILS_H

#include <string.h>

size_t get_buf_value(size_t value, size_t buf_size);
size_t decrement_buf_value(size_t value, size_t buf_size);
size_t increment_buf_value(size_t value, size_t buf_size);

#endif
