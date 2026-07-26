#include <kernel/dir_lookup_utils.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void format_to_83(char *in, char out[12]) {
  char *input = in;
  memset(out, ' ', 11);
  int x = 0;

  if (strlen(in) > 11) {
    printf("File name too big\n");
    return;
  }

  while (*input != '\0' && *input != '.' && x < 8) {
    char curr_char = *input;

    if (curr_char >= 'a' && curr_char <= 'z')
      curr_char -= 32;

    out[x++] = curr_char;
    input++;
  }

  // We know there is an extension and now need to handle it
  if (*input == '.') {
    input++;
    x = 8;

    while (*input != '\0' && x < 11) {
      char curr_char = *input;

      if (curr_char >= 'a' && curr_char <= 'z')
        curr_char -= 32;

      out[x++] = curr_char;
      input++;
    }
  }
  out[11] = '\0';
}

char *format_from_83(char in[12], char out[12]) {
  char *input = in;
  int pos = 0;
  for (int i = 0; i < 8; i++) {
    if (input[i] == ' ')
      break;

    if (input[i] == '.' || input[i] < 'A' || input[i] > 'Z') {
      out[pos++] = input[i];
    } else {
      out[pos++] = input[i] += 32;
    }
  }

  if (input[8] != ' ') {
    out[pos++] = '.';

    for (int j = 8; j < 11; j++) {
      if (input[j] == ' ')
        break;
      out[pos++] = input[j] += 32;
    }
  }

  out[pos] = '\0';

  return out;
}

uint32_t get_first_sector(uint32_t cluster, uint8_t sectors_per_cluster, uint32_t first_data_sector) {
  return ((cluster - 2) * sectors_per_cluster) + first_data_sector;
}
