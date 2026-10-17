#ifndef VIM_H
#define VIM_H

#include <kernel/buffer_utils.h>
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stddef.h>
#include <stdint.h>

typedef struct editor_state {
  int first_non_space_char; /* The position of the first character
                             as a continuous flat region. */
  int last_non_space_char;  // see above
  char *file_name_on_deck;
} editor_state_t;

void editor_handle_key_event(key_event_t key_event);
void editor_sbb_init();
void set_editor_active();
int get_editor_sbb_head(void);
void editor_sbb_init();
int editor_shift_head(int shift_by, Direction dir);
int editor_snap_to_active_line();
void (*get_editor_key_handler(void))(key_event_t key_event);
#endif
