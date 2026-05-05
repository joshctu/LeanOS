#ifndef INPUT_PROC_H
#define INPUT_PROC_H

#include <kernel/tty.h>
#include <stdbool.h>

void render_active_input_line();
bool input_proc_handle_key(key_event_t event);
void input_proc_reset();
size_t get_token_count(char *input);

#endif
