#include <stdint.h>

void strcpy(char *des, const char *src) {
  int i;

  for (i = 0; src[i] != '\0'; i++) {
    des[i] = src[i];
  }

  des[i] = '\0';
}
