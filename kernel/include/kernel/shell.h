#ifndef _SHELL_H
#define _SHELL_H

#include <kernel/input_buf.h>
#include <stdint.h>
#include <string.h>

#define PROMPT "@leanos$ "

typedef struct {
  char *prompt;
} shell_t;

void shell_init();
shell_t *get_shell_info();
int shell_read(size_t argc, char *argv[]);
size_t get_active_line();

#endif
