#include <kernel/kheap.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char *strcat(const char *str1, const char *str2) {
  int size = strlen(str1) + strlen(str2) + 1;
  char *result = kmalloc(size);

  int i = 0;
  int j = 0;

  while (str1[i] != '\0') {
    result[i] = str1[i];
    i++;
  }

  while (str2[j] != '\0') {
    result[i] = str2[j];
    i++;
    j++;
  }

  result[size - 1] = '\0';
  return result;
}
