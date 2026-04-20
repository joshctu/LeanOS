#ifndef _SHELL_H
#define _SHELL_H

#include <kernel/input_buf.h>
#include <stdint.h>
#include <string.h>

#define PROMPT "root@leanos$ "

typedef struct {
  char *prompt;

} shell_t;

void shell_init();
void shell_read(size_t argc, char *argv[]);
char *get_prompt();
size_t get_active_line();

#endif
