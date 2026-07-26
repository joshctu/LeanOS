#ifndef _DISK_H
#define _DISK_H

#include <stdbool.h>
#include <stdint.h>

// Vars with and "and" denote a dual purpose register and are listed in R/W order
// All other registers are R/W implicitly
#define DRV_DATA_REG 0x1F0
#define DRV_FEATURES_AND_ERROR_REG 0x1F1
#define DRV_SECTOR_COUNT_REG 0x1F2
#define DRV_SECTOR_NUMBER_REG 0x1F3
#define DRV_CYL_LOW_REG 0x1F4
#define DRV_CYL_HIGH_REG 0x1F5
#define DRV_SELECT_REG 0x1F6
#define DRV_STATUS_AND_COMMAND_REG 0x1F7

#define DRV_ALT_STATUS_AND_CONTROL_REG 0x3F6
#define DRV_ALT_ADDR_REG 0x3F7

#define IDENTIFY 0xEC
#define FLUSH 0xE7
#define READ_SECTORS 0x20
#define WRITE_SECTORS 0x30

typedef enum {
  FAT12,
  FAT16,
  FAT32,
} FAT_Type;

typedef struct fat_BS {
  uint8_t bootjmp[3];
  uint8_t oem_name[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sector_count;
  uint8_t table_count;
  uint16_t root_entry_count;
  uint16_t total_sectors_16;
  uint8_t media_type;
  uint16_t table_size_16;
  uint16_t sectors_per_track;
  uint16_t head_size_count;
  uint32_t hidden_sector_count;
  uint32_t total_sectors_32;
} __attribute__((packed)) fat_BS_t;

typedef struct fat_extBS_16 {
  uint8_t bios_drive_num;
  uint8_t reserved1;
  uint8_t boot_signature;
  uint32_t volume_id;
  unsigned char volume_label[11];
  unsigned char fat_type_label[8];
} __attribute__((packed)) fat_extBS_16_t;

typedef struct fat_extBS_32 {
  uint8_t file_attr;
  uint32_t file_size;
  uint32_t table_size_32;
  uint16_t extended_flags;
  uint16_t fat_version;
  uint32_t root_cluster;
  uint16_t fat_info;
  uint16_t backup_BS_sector;
  uint8_t reserved_0[12];
  uint8_t drive_number;
  uint8_t reserved_1;
  uint8_t boot_signature;
  uint32_t volume_id;
  unsigned char volume_label[11];
  unsigned char fat_type_label[8];
} __attribute__((packed)) fat_extBS_32_t;

typedef struct drive {
  uint32_t total_sectors;
  uint32_t fat_size;
  uint32_t root_dir_sectors;
  uint32_t first_data_sector;
  uint32_t first_fat_sector;
  uint32_t data_sectors;
  uint32_t total_clusters;
  uint32_t root_dir_lba;
  FAT_Type fat_type;
} __attribute__((packed)) drive_t;

typedef struct cwd {
  bool is_root;
  uint16_t cluster;
  char *name;
} cwd_t;

typedef struct dir_entry {
  char *file_name;
  uint8_t file_attr;
  uint32_t file_size;
  uint16_t first_cluster_num;
} dir_entry_t;

void identify_disk();
void write_to_disk(uint32_t LBA, uint8_t *buffer);
uint16_t *read_from_disk(uint32_t LBA);
cwd_t *get_cwd();
void read_curr_dir();
bool find_file(char *name, dir_entry_t *out);
void follow_cluster_chain(dir_entry_t file);
void dump_file(dir_entry_t file);
bool set_new_dir(char *name);
void mount_disk();
uint32_t dir_lookup(char *dir_path);
void create_dir(char *dir_name);
#endif
