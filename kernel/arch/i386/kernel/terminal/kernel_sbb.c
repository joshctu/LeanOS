#include <kernel/buffer_utils.h>
#include <kernel/input_proc.h>
#include <kernel/kernel_sbb.h>
#include <kernel/tty.h>
#include <kernel/vga.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static sbbuffer_instance_t kernel_sbb_instance;
static scrollback_buffer_t kernel_sbb = {.sbb_instance = &kernel_sbb_instance};

int get_kernel_sbb_head() { return kernel_sbb.sbb_head; }

// Initialize sbb set all entries blank
void kernel_sbb_init() { sbb_init(&kernel_sbb); }

int kernel_sbb_cursor_right() { return sbb_cursor_right(&kernel_sbb); }
// Write to buffer. Note that value here is a vga entry. Not a raw char
void kernel_sbb_write_char(uint16_t value) { sbb_write_char(&kernel_sbb, value); }

int kernel_sbb_write_line(line_t *line) { return sbb_write_line(&kernel_sbb, line); }

int kernel_sbb_new_line() { return sbb_new_line(&kernel_sbb); }

int kernel_sbb_reset() { return sbb_reset(&kernel_sbb); }

bool kernel_sbb_is_at_bottom() { return is_at_bottom(&kernel_sbb); }

bool kernel_sbb_is_at_top() { return is_at_top(&kernel_sbb); }

int kernel_shift_head(int shift_by, Direction dir) { return shift_head(&kernel_sbb, shift_by, dir); }

int kernel_snap_to_active_line() { return snap_to_active_line(&kernel_sbb); }

void kernel_viewport_sync(uint16_t *terminal_buffer) { viewport_sync(&kernel_sbb, terminal_buffer); }
