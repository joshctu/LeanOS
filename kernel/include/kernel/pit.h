#ifndef PIT_H
#define PIT_H

#include <stdint.h>

#define PIT_COMMAND 0x43
#define PIT_CHANNEL_0 0x40
#define TERMINAL_COUNT 0x30 //This value sets lo/hi byte access, terminal count mode, on channel 0.
#define SQUARE_COUNT 0x36 //This value sets lo/hi byte access, square pulse mode, on channel 0.
#define MAX_INTERVAL 0xFFFF //Max 16 bit value the timer can take



void pit_reload(uint16_t count);
void pit_mode_set(uint8_t command_flag);


#endif
