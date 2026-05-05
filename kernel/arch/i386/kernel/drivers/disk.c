#include <kernel/disk.h>
#include <kernel/io.h>
#include <stddef.h>
#include <stdio.h>

// We will control primary with ports 0x1F0 - DRIVE_STATUS_AND_COMMAND
// Errors DO NOT generate IRQ. Need to poll status register for a while after a command to be sure no error was returned.
void identify_disk() {

  // master
  outb(DRIVE_SELECT, 0xA0);

  outb(DRIVE_SECTOR_COUNT, 0);
  outb(DRIVE_SECTOR_NUMBER, 0);
  outb(DRIVE_CYL_LOW, 0);
  outb(DRIVE_CYL_HIGH, 0);

  outb(DRIVE_STATUS_AND_COMMAND, IDENTIFY);

  uint8_t does_disk_exist = inb(DRIVE_STATUS_AND_COMMAND);
  if (does_disk_exist != 0) {
    // Wait for bsy to clear
    while ((inb(DRIVE_STATUS_AND_COMMAND) & 0x80))
      ;

    // Wait for either ready or error
    while (!(inb(DRIVE_STATUS_AND_COMMAND) & 0x08) || !inb(DRIVE_STATUS_AND_COMMAND) & 0x1)
      ;
    if (inb(DRIVE_STATUS_AND_COMMAND) & 0x08) {
      printf("Drive good to go");
    }
    if (inb(DRIVE_STATUS_AND_COMMAND) & 0x1) {
      printf("Drive error");
    }
  }
}
