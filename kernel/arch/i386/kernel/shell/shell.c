#include <stdint.h>
#include <stdio.h>

#include <kernel/input_proc.h>
#include <kernel/shell.h>
#include <kernel/tty.h>

shell_t shell;

void shell_init() {
  terminal_setcolor(4);
  printf("\nInitializing shell... [Ok]\n");

  shell.prompt = PROMPT;

  active_program_t *active_program = get_active_program();

  active_program->shell = &shell;
  input_proc_reset();
}

void shell_read(size_t argc, char *argv[]) {
  if (argc == 0)
    return;

  // if (*argv[0] == "clear") {
  //   terminal_clear_screen();
  //  }
}

void shell_execute() {
  // try to match string to prompt
  // write prompt and user input to terminal
  // new line
  // generate new prompt
}
