## Quick Start

### Prerequisites

- An i686-elf GCC/binutils cross-compiler on your PATH (i686-elf-gcc)
- qemu-system-i386, qemu-img
- grub-mkrescue (with xorriso and mtools)
- dosfstools (mkfs.fat)
- A Unix shell, make, and sudo (the disk setup mounts a loop device)

### Build

./build.sh      # builds libc and the kernel into ./sysroot
./iso.sh        # builds, then creates LEANOS.iso with GRUB
./clean.sh removes build output.

### Run

./qemu.sh
This builds the ISO, creates the virtual disk, and starts QEMU with a Pentium III CPU, the ISO as the boot CD and the disk on an IDE controller. The QEMU monitor is on stdio. drive.sh creates a 50 MB FAT16 image (virt_drive.img) the first time and seeds it with /home and a few sample files. It does nothing if the image exists. Delete virt_drive.img to reset the filesystem.

Shell Commands
 | Command | Usage                            | Description                                                 |
 |---------|--------------------------------- |-------------------------------------------------------------|
 | help    | help                             | List available commands                                     | 
 | clear   | clear                            | Clear the screen                                            |
 | echo    | echo <text>                      | Print args                                                  |
 | ls      | ls                               | List current directory                                      |
 | cd      | cd <dir>                         | Change directory                                            | 
 | find    | find <name>                      | Show a file or directory's type, size, and starting cluster |
 | cat     | cat <file>                       | Print a file's contents                                     |
 | mkdir   | mkdir <name>                     | Create a directory (with . and .. references)               | 
 | touch   | touch <name>                     | Create an emtpy file                                        |
 | mmap    | mmap rgndmp / mmap rgngen <size> | Dump allocated memory regions, or allocate runtime region   |
 | heap    | heap dump                        | Dump kernel heap allocations                                | 
 | vim     | vim                              | In progress                                                 |

Project Layout

```
├── isodir
│   └── boot
│       └── grub
├── kernel
│   ├── arch
│   │   └── i386
│   │       ├── kernel
│   │       │   ├── drivers
│   │       │   │   └── fat_disk
│   │       │   ├── interrupts
│   │       │   ├── logs
│   │       │   ├── memory
│   │       │   │   └── kernel_heap
│   │       │   ├── paging
│   │       │   ├── shell
│   │       │   ├── terminal
│   │       │   └── vim
│   │       └── utils
│   ├── include
│   │   └── kernel
│   └── kernel
├── libc
│   ├── arch
│   │   └── i386
│   ├── include
│   │   └── sys
│   ├── stdio
│   ├── stdlib
│   └── string
└── sysroot
    ├── boot
    └── usr
        ├── include
        │   ├── kernel
        │   └── sys
        └── lib
```
## Architecture

### Boot Process
<!-- GRUB/Multiboot2 handoff, early assembly, jump to C, GDT setup. -->

#### Linker
The kernel is linked at 3GB but loaded by GRUB at 1MB. In the linker, we place the multiboot header first so it can be found by GRUB in the first 8KB. We then link at 3GB while placing the load address at 1MB. 
This allows our early boot code to run naturally at its physical location, while requiring everything after it to be referenced with a 0x30000000 offset. 

#### Early Boot
The early boot code is responsible for setting up paging right from the get-go. 

 Boot Sequence:
 - Load at 1MB and jump to _start
 - Build identity mapping for paging using page directory entry 0 (identity map) and 768 (higher half).
 - Map vga buffer and recursive directory entry. 
 - Load page directories physical address in cr3
 - Enable paging with cr0 and absolute jump to higher half
 - Reload cr3 to flush TLB
 - Get rid of identity mapping, push multiboot info pointer and magic number onto the stack and call kernel_main


### Memory Management
<!-- Physical memory tracking, paging, higher-half mapping, recursive page directory entry, VMM allocation, kernel heap. -->
#### Physical Memory Manager
GRUB conveniently supplies an info struct to the kernel that details which regions in RAM are available and which are reserved.
The PMM iterates through this struct to build a bitmap of reserved page frames (4 KB regions of memory). Every frame has a corresponding entry in the bitmap of either 0 or 1 for free and reserved. 
These 4 KB regions are aligned up or down to ensure 4 KB alignment. 
Symbols declared in the linker are used to calculate kernel size, and the appropriate frames are then reserved.

#### Virtual Memory Manager
The VMM maintains a linked list of blocks that describe currently allocated virtual memory regions and their flags. On initialization, the VMM will iterate the paging structure using the recursive entry
and look for the present bit to initialize regions of allocated memory. When new memory is requested the VMM will look through its region map to find a suitable range. When found, it will request physical pages from the PMM 
and map them to virtual addresses.

#### Heap
`kmalloc`/`kfree` are backed by a linked list of heap blocks obtained from the VMM
in multiples of 4 KB. Each block is divided into 16-byte subblocks, and a header plus
a bitmap track which subblocks are in use.

An allocation takes a contiguous run of free subblocks and tags each bitmap entry
with a unique allocation id. `kfree` looks up the id of the first subblock the pointer
refers to and clears every subblock with the same id, so the allocator needs no
per-allocation size field.

#### Virtual Address Layout
<!-- Table: region | address range | purpose (kernel, user, recursive mapping, heap). -->
### Interrupts and Drivers
<!-- IDT, PIC/PIT, keyboard, port I/O, terminal/TTY. -->
### Filesystem (FAT16)
<!-- Disk access, on-disk layout, cluster allocation, directory traversal, supported operations and known limits. -->
### Shell
<!-- Input handling, command dispatch, scrollback buffer, editor if finished. -->

## Design Decisions and Challenges



## Testing and Debugging


## Limitations and Known Issues

## References
<!-- OSDev wiki pages, Intel manuals, FAT specification, anyth
## License
