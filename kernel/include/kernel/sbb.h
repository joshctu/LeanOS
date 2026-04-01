#ifndef _SBB_H
#define _SBB_H
#include <stdint.h>
#include <stddef.h>

#define SBB_SIZE 1000

typedef uint16_t cell_t;
typedef cell_t line_t[80];
typedef line_t scrollback_buffer_t[SBB_SIZE];

void sbb_init(void);
void viewport_sync(uint16_t *terminal_buffer);
void sbb_write(size_t cursor_pos, uint16_t value);
void shift_head(size_t cursor_pos);


#endif
