#include <kernel/buffer_utils.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <kernel/vim.h>
#include <stddef.h>
#include <stdint.h>

#define NO_POS -1
#define EDITOR_WIDTH 80

static editor_state_t editor_state = {.first_non_space_char = NO_POS, .last_non_space_char = NO_POS};
static sbbuffer_instance_t editor_sbb_instance;
static scrollback_buffer_t editor_sbb = {.sbb_instance = &editor_sbb_instance, .initialized = 0};

void editor_sbb_init() { sbb_init(&editor_sbb); }

void editor_handle_key_event(key_event_t key_event) {

void set_new_first_non_space_char() {
  /* Simple case of first character */
  if (editor_state.first_non_space_char == NO_POS) {
    editor_state.first_non_space_char = get_editor_cursor_pos();
    return;
  }

  /* Harder case of seeking forward to the next character */
  for (int i = editor_state.first_non_space_char; i <= editor_state.last_non_space_char; ++i) {
    unsigned char ch = (uint16_t)(*editor_sbb.sbb_instance)[i / EDITOR_WIDTH][i % EDITOR_WIDTH] & 0xFF;
    if (ch != ' ') {
      editor_state.first_non_space_char = i;
      return;
    }
  }

  /* Editor was empty reset */
  editor_state.first_non_space_char = NO_POS;
  editor_state.last_non_space_char = NO_POS;
}

void set_new_last_non_space_char() {
  int cursor_pos = get_editor_cursor_pos();

  /* Simple case of first character or writing past the previous lasts character */
  if (editor_state.last_non_space_char == NO_POS || cursor_pos > editor_state.last_non_space_char) {
    editor_state.last_non_space_char = cursor_pos;
    return;
  }

  /* Harder case where we need to seek back to last character */
  for (int i = editor_state.last_non_space_char; i >= editor_state.first_non_space_char; --i) {
    unsigned char ch = (uint16_t)(*editor_sbb.sbb_instance)[i / EDITOR_WIDTH][i % EDITOR_WIDTH] & 0xFF;
    if (ch != ' ') {
      editor_state.last_non_space_char = i;
      return;
    }
  }

  /* Editor was empty reset */
  editor_state.first_non_space_char = NO_POS;
  editor_state.last_non_space_char = NO_POS;
}

  /* Process key events from the terminal */
  switch (key_event.key) {
  case KEY_CHAR:
    if (key_event.ch == ':') {
      terminal_reset_owner();
      break;
    }
    terminal_write_fwd(key_event.ch);
    if (key_event.ch != ' ') {
      set_new_last_non_space_char();
      set_new_first_non_space_char();
    }
    break;
  case KEY_ENTER:
    terminal_new_line();
    break;
  case KEY_BACKSPC:
    terminal_write_bkwd(' ');
    int cursor_pos = get_editor_cursor_pos();
    if (cursor_pos == editor_state.first_non_space_char) {
      set_new_first_non_space_char();
    }
    set_new_last_non_space_char();
    break;
  case KEY_LEFT_ARROW:
    terminal_cursor_left();
    break;
  case KEY_RIGHT_ARROW:
    terminal_cursor_right();
    break;
  case KEY_DOWN_ARROW:
    terminal_cursor_down();
    break;
  case KEY_UP_ARROW:
    terminal_cursor_up();
  }
}

void (*get_editor_key_handler(void))(key_event_t key_event) { return editor_handle_key_event; }

void set_editor_active() {
  editor_sbb_init();
  set_terminal_owner(&editor_sbb, get_editor_key_handler());
}

int get_editor_cursor_pos() { return (editor_sbb.sbb_row_idx * EDITOR_WIDTH) + editor_sbb.sbb_col_idx; }



// int get_editor_sbb_head(void);
// void editor_sbb_init();
//  int editor_shift_head(int shift_by, Direction dir);
// int editor_snap_to_active_line();
