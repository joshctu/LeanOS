#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <kernel/input_proc.h>
#include <kernel/shell.h>
#include <kernel/tty.h>

shell_t shell;

const char *command_list = "Available commands...\nhelp, clear, echo";

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

  if (argc == 1) {
    if (strcmp(argv[0], "clear"))
      terminal_clear_screen();

    if (strcmp(argv[0], "help"))
      printf("%s\n", command_list);

    return;
  }
  // Single keyword commands
  if (strcmp(argv[0], "echo")) {
    for (size_t i = 1; i < argc; i++) {
      printf("%s ", argv[i]);
    }
    printf("\n");
  }
}

void shell_execute() {
  // try to match string to prompt
  // write prompt and user input to terminal
  // new line
  // generate new prompt
}
