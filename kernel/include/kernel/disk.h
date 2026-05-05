#ifndef _DISK_H
#define _DISK_H

// Vars with and "and" denote a dual purpose register and are listed in R/W order
// All other registers are R/W implicitly
#define DRIVE_DATA 0x1F0
#define DRIVE_FEATURES_AND_ERROR 0x1F1
#define DRIVE_SECTOR_COUNT 0x1F2
#define DRIVE_SECTOR_NUMBER 0x1F3
#define DRIVE_CYL_LOW 0x1F4
#define DRIVE_CYL_HIGH 0x1F5
#define DRIVE_SELECT 0x1F6
#define DRIVE_STATUS_AND_COMMAND 0x1F7

#define IDENTIFY 0xEC

void identify_disk();
#endif
