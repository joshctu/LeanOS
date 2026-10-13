#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <kernel/buffer_utils.h>
#include <kernel/shell.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SBB_SIZE 1000

typedef enum {
  KEY_CHAR,
  KEY_ENTER,
  KEY_BACKSPC,
  KEY_PG_UP,
  KEY_PG_DOWN,
  KEY_LEFT_ARROW,
  KEY_RIGHT_ARROW,
  KEY_DOWN_ARROW,
  KEY_UP_ARROW,
} KeyEvent;

typedef struct {
  KeyEvent key;
  char ch;
} key_event_t;

typedef struct {
  const char *prompt;
  const char *buf;
  int size;
  int cursor;
} tty_line_t;

typedef struct {
  void (*handler_fxn)(key_event_t key_event);
  void *context;
} tty_owner_t;

uint16_t *get_terminal_buffer();
uint8_t get_terminal_color();

void set_terminal_owner(scrollback_buffer_t *sbb, void (*handler_fxn)(key_event_t key_event));
void terminal_reset_owner();
void terminal_initialize(void);
void terminal_set_cursor(size_t pos);
void terminal_write(const char *data, size_t size);
void terminal_setcolor(uint8_t color);
void terminal_handle_key(key_event_t event);
void terminal_render_line(const tty_line_t *line);
void terminal_commit_line(const tty_line_t *line);
void terminal_clear_screen();

int viewport_sync(scrollback_buffer_t *sbb);

void terminal_write_fwd(char c);
void terminal_write_bkwd(char c);
void terminal_new_line();

void terminal_cursor_right();
void terminal_cursor_left();
void terminal_cursor_up();
void terminal_cursor_down();

void handle_page_up();
void handle_page_down();

#endif
