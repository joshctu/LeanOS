#ifndef _SBB_H
#define _SBB_H
#include "buffer_utils.h"
#include <kernel/tty.h>
#include <stddef.h>
#include <stdint.h>

scrollback_buffer_t *get_shell_sbb();
void (*get_shell_key_handler(void))(key_event_t key_event);
void tokenize_and_send_to_shell(char *input);
void input_proc_reset();
void render_active_input_line();
void shell_handler_key_event(key_event_t key_event);
int get_kernel_sbb_head(void);
void kernel_sbb_init();
int kernel_sbb_write_line(tty_line_t *line);
int kernel_shift_head(int shift_by, Direction dir);
int kernel_snap_to_active_line();

#endif
