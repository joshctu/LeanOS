#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int count_digits(int input) {

  int count = 0;

  do {
    count++;
    input /= 10;
  } while (input != 0);

  return count;
}

char *itoa(int input, char *buffer) {

  int data = input;
  bool isNeg = false;

  if (data == 0) {
    buffer[0] = '0';
    buffer[1] = '\0';
    return buffer;
  }

  if (data < 0) {
    isNeg = true;
    data = -data;
  }

  int numberOfDigits = count_digits(data);

  if (isNeg) {
    numberOfDigits = numberOfDigits + 1;
  }

  buffer[numberOfDigits] = '\0';

  for (int i = numberOfDigits - 1; i >= 0; i--) {

    if (i == 0 && isNeg) {
      buffer[i] = '-';
      break;
    }

    int nextDigit = data % 10;
    char charOfInt = '0' + nextDigit;
    buffer[i] = charOfInt;
    data = data / 10;
  }

  return buffer;
}

char *itoa_hex(uint32_t input, char *buffer) {

  uint32_t data = input;
  int i = 10;
  buffer[i] = '\0';
  i--;

  do {

    int digit = data % 16;
    if (digit < 10) {
      buffer[i] = '0' + digit;
    } else {
      buffer[i] = 'a' + (digit - 10);
    }

    data = data / 16;
    i--;
  } while (i >= 2);

  buffer[0] = '0';
  buffer[1] = 'x';

  return buffer;
}

static bool print(const char *data, size_t length) {
  const unsigned char *bytes = (const unsigned char *)data;
  for (size_t i = 0; i < length; i++) {

    if (putchar(bytes[i]) == EOF)
      return false;
  }
  return true;
}

int printf(const char *restrict format, ...) {
  va_list parameters;
  va_start(parameters, format);

  int written = 0;

  while (*format != '\0') {
    size_t maxrem = INT_MAX - written;
    if (format[0] != '%' || format[1] == '%') {
      if (format[0] == '%')
        format++;
      size_t amount = 0;
      while (format[amount] && format[amount] != '%')
        amount++;
      if (maxrem < amount) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!print(format, amount))
        return -1;
      format += amount;
      written += amount;
      continue;
    }

    const char *format_begun_at = format++;

    if (*format == 'c') {
      format++;
      char c = (char)va_arg(parameters, int /* char promotes to int */);
      if (!maxrem) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!print(&c, sizeof(c)))
        return -1;
      written++;
    }

    else if (*format == 'd') {
      format++;
      char buffer[12];
      const char *str = itoa(va_arg(parameters, int), buffer);
      size_t len = strlen(str);
      if (maxrem < len) {
        return -1;
      }

      if (!print(str, len))
        return -1;
      written += len;
    }

    else if (*format == 'x') {
      format++;
      char buffer[11];
      memset(buffer, '0', 10);
      buffer[10] = '\0';
      const char *str = itoa_hex(va_arg(parameters, uint32_t), buffer);
      size_t len = strlen(str);
      if (maxrem < len) {
        return -1;
      }
      if (!print(str, len)) {
        return -1;
      }
      written += len;
    }

    else if (*format == 's') {
      format++;
      const char *str = va_arg(parameters, const char *);
      size_t len = strlen(str);
      if (maxrem < len) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!print(str, len))
        return -1;
      written += len;
    } else {
      format = format_begun_at;
      size_t len = strlen(format);
      if (maxrem < len) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!print(format, len))
        return -1;
      written += len;
      format += len;
    }
  }

  va_end(parameters);
  return written;
}
