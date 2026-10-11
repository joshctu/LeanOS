#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

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

void terminal_initialize(void);
void terminal_set_cursor(size_t pos);
void terminal_write(const char *data, size_t size);
void terminal_setcolor(uint8_t color);
void terminal_handle_key(key_event_t event);
void terminal_render_line(const tty_line_t *line);
void terminal_commit_line(const tty_line_t *line);
void terminal_clear_screen();

void write_fwd(char c);
void new_line();

void cursor_right();

void handle_page_up();
void handle_page_down();

#endif
