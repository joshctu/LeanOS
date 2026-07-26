#ifndef _DIR_LOOKUP_UTILS
#define _DIR_LOOKUP_UTILS

#include <stdint.h>

void format_to_83(char *in, char out[12]);
char *format_from_83(char in[12], char out[12]);
uint32_t get_first_sector(uint32_t cluster, uint8_t sectors_per_cluster, uint32_t first_data_sector);
#endif
