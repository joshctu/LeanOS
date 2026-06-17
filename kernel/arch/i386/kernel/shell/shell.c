#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/disk.h>
#include <kernel/input_proc.h>
#include <kernel/kheap.h>
#include <kernel/shell.h>
#include <kernel/tty.h>
#include <kernel/vmm.h>

shell_t shell;

const char *command_list = "Available commands...\nhelp, clear, echo";

void shell_init() {
  terminal_setcolor(4);
  printf("\nInitializing shell... [Ok]\n");

  shell.prompt = PROMPT;

  active_program_t *active_program = get_active_program();

  active_program->shell = &shell;
  active_program->program_set = true;
  input_proc_reset();
}

void shell_read(size_t argc, char *argv[]) {
  if (argc == 0)
    return;

  if (strcmp(argv[0], "clear")) {
    terminal_clear_screen();
    return;
  }
  if (strcmp(argv[0], "help")) {
    printf("%s\n", command_list);
    return;
  }
  if (strcmp(argv[0], "ls")) {
    read_root_dir();
    return;
  }

  if (strcmp(argv[0], "echo") && argc > 1) {
    for (size_t i = 1; i < argc; i++) {
      printf("%s ", argv[i]);
    }
    printf("\n");
  }

  if (strcmp(argv[0], "find") && argc > 1) {
    dir_entry_t new_entry;
    if (find_in_root(argv[1], &new_entry)) {
      char *file_type = new_entry.file_attr == 0x10 ? "directory" : "file";
      printf("file name: %s\nfile type: %s\nfile size: %d bytes\nfirst cluster number: %d\n", new_entry.file_name, file_type, new_entry.file_size,
             new_entry.first_cluster_num);
    }
  }

  if (strcmp(argv[0], "cat") && argc > 1) {
    dir_entry_t new_entry;
    if (find_in_root(argv[1], &new_entry)) {
      follow_cluster_chain(new_entry);
    }
  }

  if (strcmp(argv[0], "mmap")) {
    if (argc > 1 && strcmp(argv[1], "rgngen")) {
      if (argc > 2) {
        char *size_arg = argv[2];
        char *somestring = kmalloc(atoi(size_arg));
      }
      return;
    }
    if (argc > 1 && strcmp(argv[1], "rgndmp")) {
      print_all_regions();
      return;
    }
  }

  if (strcmp(argv[0], "heap")) {
    if (argc > 1 && strcmp(argv[1], "dump")) {
      kheap_reg_dump();
    }
  }
}
