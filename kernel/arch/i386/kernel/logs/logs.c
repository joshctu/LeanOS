#include <kernel/logs.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <kernel/mmap.h>

char *boot_banner =
"//  __       ____    ______  __  __  _____   ____       \n"
"// /\\ \\     /\\  _`\\ /\\  _  \\/\\ \\/\\ \\/\\  __`\\/\\  _`\\     \n"
"// \\ \\ \\    \\ \\ \\L\\_\\ \\ \\L\\ \\ \\ `\\\\ \\ \\ \\/\\ \\ \\,\\L\\_\\   \n"
"//  \\ \\ \\  __\\ \\  _\\L\\ \\  __ \\ \\ , ` \\ \\ \\ \\ \\/_\\__ \\   \n"
"//   \\ \\ \\L\\ \\\\ \\ \\L\\ \\ \\ \\/\\ \\ \\ \\`\\ \\ \\ \\_\\ \\/\\ \\L\\ \\ \n"
"//    \\ \\____/ \\ \\____/\\ \\_\\ \\_\\ \\_\\ \\_\\ \\_____\\ `\\____\\\n"
"//     \\/___/   \\/___/  \\/_/\\/_/\\/_/\\/_/\\/_____/\\/_____/\n";

void print_header_log(char *s) {
	terminal_setcolor(14);
	printf("%s", s);
}

void print_body_log(char *s) {
	terminal_setcolor(2);
	printf("%s", s);
}
void generate_boot_logs(multiboot_info_t *mbi) {

	print_header_log("Welcome to:\n");
	print_body_log(boot_banner);
	print_header_log("Detecting system info...\n");
	uint32_t tot_mem = detect_available_mem(mbi);
	terminal_setcolor(2);
	printf("Ram available: %dKB / %dKB\n", (tot_mem / 1024 - (get_kernel_size() / 1024 )) ,tot_mem / 1024);

	uint32_t cr0;
	asm volatile("movl %%cr0, %0" :"=r"(cr0));

	print_header_log("Detecting paging status...\n");
	if ((cr0 >> 31) & 0x1) {
		print_body_log("Paging enabled\n");
	}
	else {
		print_body_log("Paging disabled\n");
	}
	if (mbi->flags >> 2 & 0x1) {
		print_header_log("Boot args...\n");
		terminal_setcolor(2);
		printf("%s\n", (char*)(mbi->cmdline + 0xC0000000));
	}


}
