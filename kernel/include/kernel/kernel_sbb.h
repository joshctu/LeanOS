#ifndef _SBB_H
#define _SBB_H
#include "buffer_utils.h"
#include <stddef.h>
#include <stdint.h>

int get_kernel_sbb_head(void);
void kernel_sbb_init();
int kernel_sbb_cursor_right();
void kernel_sbb_write_char(uint16_t value);
int kernel_sbb_write_line(line_t *line);
int kernel_sbb_new_line();
int kernel_sbb_reset();
bool kernel_sbb_is_at_bottom();
bool kernel_sbb_is_at_top();
int kernel_shift_head(int shift_by, Direction dir);
int kernel_snap_to_active_line();
void kernel_viewport_sync(uint16_t *terminal_buffer);
#endif
