#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>
#include <stdint.h>

#define SBB_SIZE 1000
#define SHELL_BUF_SIZE 60

typedef struct {
	char data[SHELL_BUF_SIZE];
	uint8_t head;
	uint8_t tail;
	uint8_t size;
} stdin_buffer_t;

stdin_buffer_t *get_stdin_buffer();
void stdin_buffer_write(char c);
void stdin_buffer_bck();
void stdin_buffer_clear();
char stdin_buffer_read();

void terminal_initialize(void);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_setcolor(uint8_t color);

void set_input_start(int idx);

void write_fwd(char c);
void write_bck();
void new_line();

void cursor_up();
void cursor_down();
void cursor_left();
void cursor_right();
void update_cursor(void);
size_t get_cursor(void);

#endif
