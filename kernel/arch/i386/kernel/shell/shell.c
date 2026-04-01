#include <kernel/tty.h>
#include <stdint.h>
#include <stdio.h>

char *prompt = "root@leanos$ ";

char *get_prompt() { return prompt; }
void generate_prompt() { printf("\n%s", prompt); }

char *trim_leading_whitespace(char *str) {
}

char *trim_trailing_whitespace(char *str) {
}

char *trim_middling_whitespace(char *str) {
}

void shell_init() { 
	printf("\nInitializing shell... [Ok]");
	generate_prompt(); 
}

void shell_read() {
	stdin_buffer_t *stdin_buffer = get_stdin_buffer();
	char raw_input[stdin_buffer->size + 1];
	for (size_t i = 0; i < stdin_buffer->size + 1; i++) {
		raw_input[i] = stdin_buffer_read();
	}
	printf("\n%s", raw_input);
	generate_prompt();
}

void shell_execute() {
  // try to match string to prompt
  // write prompt and user input to terminal
  // new line
  // generate new prompt
}
