#include <kernel/buffer_utils.h>
#include <kernel/tty.h>
#include <kernel/vim.h>
#include <stddef.h>
#include <stdint.h>

static sbbuffer_instance_t editor_sbb_instance;
static scrollback_buffer_t editor_sbb = {.sbb_instance = &editor_sbb_instance, .initialized = 0};

void editor_sbb_init() { sbb_init(&editor_sbb); }

void editor_handle_key_event(key_event_t key_event) {

  /* Process key events from the terminal */
  switch (key_event.key) {
  case KEY_CHAR:
    if (key_event.ch == ':') {
      terminal_reset_owner();
      break;
    }
    terminal_write_fwd(key_event.ch);
    break;
  case KEY_ENTER:
    terminal_new_line();
    break;
  case KEY_BACKSPC:
    terminal_write_bkwd(' ');
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

// int get_editor_sbb_head(void);
// void editor_sbb_init();
//  int editor_shift_head(int shift_by, Direction dir);
// int editor_snap_to_active_line();
