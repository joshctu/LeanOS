# Declare constants for the multiboot header.
.set ALIGN,    1<<0            
.set MEMINFO,  1<<1             
.set FLAGS,    ALIGN | MEMINFO 
.set MAGIC,    0x1BADB002       
.set CHECKSUM, -(MAGIC + FLAGS) 

# Declare a multiboot header that marks the program as a kernel.
.section .multiboot.data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Allocate the initial stack.
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

# Preallocate pages used for paging. Don't hard-code addresses and assume they
# are available, as the bootloader might have loaded its multiboot structures or
# modules there. This lets the bootloader know it must avoid the addresses.
.section .bss, "aw", @nobits
	.align 4096
boot_page_directory:
	.skip 4096
boot_page_table1:
	.skip 4096
# Further page tables may be required if the kernel grows beyond 3 MiB.

# The kernel entry point.
.section .multiboot.text, "a"
.global _start
.type _start, @function
_start:
	movl $(boot_page_table1 - 0xC0000000), %edi
	movl $0, %esi
	movl $1024, %ecx

1:
   	movl %esi, %edx
    	orl $0x003, %edx
    	movl %edx, (%edi)

    	addl $4096, %esi
    	addl $4, %edi
    	loop 1b

2:
	# Map VGA video memory to 0xC03FF000 as "present, writable".
	movl $(0x000B8000 | 0x003), boot_page_table1 - 0xC0000000 + 1023 * 4

	movl $(boot_page_table1 - 0xC0000000 + 0x003), boot_page_directory - 0xC0000000 + 0
	movl $(boot_page_table1 - 0xC0000000 + 0x003), boot_page_directory - 0xC0000000 + 768 * 4
  // Recursive directory entry
	movl $(boot_page_directory - 0xC0000000 + 0x003), boot_page_directory - 0xC0000000 + 1023 * 4

	movl $(boot_page_directory - 0xC0000000), %ecx
	movl %ecx, %cr3

	movl %cr0, %ecx
	orl $0x80010000, %ecx
	movl %ecx, %cr0

	# Jump to higher half with an absolute jump.
	lea 3f, %ecx
	jmp *%ecx

.section .text

3:
	# At this point, paging is fully set up and enabled.


	# Unmap the identity mapping as it is now unnecessary.
	movl $0, boot_page_directory + 0

	# Reload cr3 to force a TLB flush so the changes to take effect.
	movl %cr3, %ecx
	movl %ecx, %cr3

	# Set up the stack.
	mov $stack_top, %esp

	pushl %ebx
	pushl %eax

	# Enter the high-level kernel.
	call kernel_main

	# Infinite loop if the system has nothing more to do.
	cli
1:	hlt
	jmp 1b
