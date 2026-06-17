#include <stddef.h>
#include <string.h>

char *trim_prefixed_whitespace(char *input) {
  char *ptr = input;

  while (*ptr == ' ')
    ptr++;

  return ptr;
}

char *trim_prefixed_slash(char *input) {
  char *ptr = input;

  while (*ptr == '/')
    ptr++;
  return ptr;
}

char *trim_trailing_slash(char *input) {
  size_t len = strlen(input);

  if (len == 0) {
    return input;
  }

  char *ptr = input + len - 1;

  while (*ptr == '/') {
    ptr--;
  }

  *(ptr + 1) == '\0';
  return input;
}

char *trim_trailing_whitespace(char *input) {
  size_t len = strlen(input);

  if (len == 0) {
    return input;
  }

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

char *trim_middling_slash(char *input) {
  size_t len = strlen(input);

  if (len == 0)
    return input;

  char *ptr_read = input;
  char *ptr_write = input;

  while (*ptr_read != '\0') {
    if (*ptr_read == '/') {
      *ptr_write = *ptr_read;
      ptr_read++;
      ptr_write++;
      while (*ptr_read == '/') {
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
