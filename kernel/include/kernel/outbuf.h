#ifndef _OUTBUF_H_
#define _OUTBUF_H_

#include <stdint.h>

#define OUTPUT_BUFFER_SIZE 256

typedef struct {
	char data[OUTPUT_BUFFER_SIZE];
	uint8_t head;
	uint8_t tail;	
	uint8_t size;

} outbuf_t;


void buffer_write(char c);
void guard_cursor_left();
void guard_cursor_right();
void handle_shell_submit();




#endif
