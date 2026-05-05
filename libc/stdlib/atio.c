#include <stdint.h>
#include <stdlib.h>

int atoi(const char *str) {
  int x;
  char *strstart;
  int result = 0;

  for (strstart = str; strstart == ' '; strstart++)
    ;

  for (x = 0; strstart[x] != '\0'; x++) {
    result = result * 10 + (strstart[x] - '0');
  }

  return result;
}
