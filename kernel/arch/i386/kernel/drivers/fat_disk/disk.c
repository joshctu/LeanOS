#include <kernel/dir_lookup_utils.h>
#include <kernel/disk.h>
#include <kernel/io.h>
#include <kernel/kheap.h>
#include <kernel/string_utils.h>
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint16_t *identify_info_buffer;
static fat_BS_t drive_mount;
static cwd_t cwd;
static fat_extBS_16_t extended_drive_mount_16;
static fat_extBS_32_t extended_drive_mount_32;
static drive_t curr_drive;
// We will control primary with ports 0x1F0 - DRV_STATUS_AND_COMMAND_REG
// Errors DO NOT generate IRQ. Need to poll status register for a while after a command to be sure no error was returned.
//
uint16_t get_disk_status() {
  // Read from alterate status reg
  uint16_t status = inw(DRV_ALT_STATUS_AND_CONTROL_REG);

  return status;
}

int flush_cache_and_wait_for_rdy() {
  outb(DRV_STATUS_AND_COMMAND_REG, FLUSH);

  for (int x = 0; x < 1000000; x++) {
    uint16_t status = get_disk_status();

    // BSY
    if (status & (0x1 << 7))
      continue;

    // ERR
    if (status & 0x1)
      continue;

    return 1;
  }

  return 0;
}

void identify_disk() {
  // master
  outb(DRV_SELECT_REG, 0xA0);

  outb(DRV_SECTOR_COUNT_REG, 0);
  outb(DRV_SECTOR_NUMBER_REG, 0);
  outb(DRV_CYL_LOW_REG, 0);
  outb(DRV_CYL_HIGH_REG, 0);

  outb(DRV_STATUS_AND_COMMAND_REG, IDENTIFY);

  uint8_t does_disk_exist = inb(DRV_STATUS_AND_COMMAND_REG);
  if (does_disk_exist != 0) {
    // Wait for bsy to clear
    while ((inb(DRV_STATUS_AND_COMMAND_REG) & 0x80))
      ;

    // Wait for either ready or error
    while (!(inb(DRV_STATUS_AND_COMMAND_REG) & 0x08) || !inb(DRV_STATUS_AND_COMMAND_REG) & 0x1)
      ;
    if (inb(DRV_STATUS_AND_COMMAND_REG) & 0x08) {

      identify_info_buffer = kmalloc(256 * sizeof(uint16_t));
      for (size_t i = 0; i < 256; i++) {
        identify_info_buffer[i] = inw(DRV_DATA_REG);
      }
      flush_cache_and_wait_for_rdy();
    }
    if (inb(DRV_STATUS_AND_COMMAND_REG) & 0x1) {
      printf("Drive error");
    }
  }
}

// This input buffer must be on the heap!!
void write_to_disk(uint32_t LBA, uint8_t *buffer) {
  // Select master drive and OR highest 4 bits of LBA
  outb(DRV_SELECT_REG, 0xE0 | ((LBA >> 24) & 0x0F));
  outb(DRV_SECTOR_COUNT_REG, (uint8_t)1);
  outb(DRV_SECTOR_NUMBER_REG, (uint8_t)LBA);
  outb(DRV_CYL_LOW_REG, (uint8_t)(LBA >> 8));
  outb(DRV_CYL_HIGH_REG, (uint8_t)(LBA >> 16));
  outb(DRV_STATUS_AND_COMMAND_REG, WRITE_SECTORS);

  uint8_t status = inb(DRV_STATUS_AND_COMMAND_REG);
  do {
    status = inb(DRV_STATUS_AND_COMMAND_REG);
  } while (status & 0x80 && !(status & 0x08));

  uint16_t *data = (uint16_t *)buffer;

  for (int x = 0; x < 256; x++) {
    outw(DRV_DATA_REG, data[x]);
  }
  flush_cache_and_wait_for_rdy();
}

uint16_t *read_from_disk(uint32_t LBA) {
  // Select master drive and OR highest 4 bits of LBA
  outb(DRV_SELECT_REG, 0xE0 | ((LBA >> 24) & 0x0F));
  outb(DRV_SECTOR_COUNT_REG, (uint8_t)1);
  outb(DRV_SECTOR_NUMBER_REG, (uint8_t)LBA);
  outb(DRV_CYL_LOW_REG, (uint8_t)(LBA >> 8));
  outb(DRV_CYL_HIGH_REG, (uint8_t)(LBA >> 16));
  outb(DRV_STATUS_AND_COMMAND_REG, READ_SECTORS);

  uint8_t status = inb(DRV_STATUS_AND_COMMAND_REG);
  do {
    status = inb(DRV_STATUS_AND_COMMAND_REG);
  } while (status & 0x80 && !(status & 0x08));

  uint16_t *buffer = kmalloc(sizeof(char) * 512);
  for (int x = 0; x < 256; x++) {
    buffer[x] = inw(DRV_DATA_REG);
  }

  return buffer;
}

void mount_disk() {

  uint8_t *BPB = (uint8_t *)read_from_disk(0);
  // Get bytes per sector
  uint8_t b11 = BPB[11];
  uint8_t b12 = BPB[12];
  uint16_t bytes_per_sector = b12 << 8 | b11;
  drive_mount.bytes_per_sector = bytes_per_sector;

  // Get sectors per cluster
  uint8_t b13 = BPB[13];
  drive_mount.sectors_per_cluster = b13;

  // Get reserved sectors
  uint8_t b14 = BPB[14];
  uint8_t b15 = BPB[15];
  uint16_t reserved_sectors = b15 << 8 | b14;
  drive_mount.reserved_sector_count = reserved_sectors;

  // Get FATs
  uint8_t b16 = BPB[16];
  drive_mount.table_count = b16;

  // Get # of root directory entries
  uint8_t b17 = BPB[17];
  uint8_t b18 = BPB[18];
  uint16_t root_entry_count = b18 << 8 | b17;
  drive_mount.root_entry_count = root_entry_count;

  // Get sectors for drives with less than 65535 sectors
  uint8_t b19 = BPB[19];
  uint8_t b20 = BPB[20];
  uint16_t total_sectors_16 = b20 << 8 | b19;
  drive_mount.total_sectors_16 = total_sectors_16;

  // Get media descriptor byte
  uint8_t b21 = BPB[21];
  drive_mount.media_type = b21;

  // Get sectors per FAT 12/16 only
  uint8_t b22 = BPB[22];
  uint8_t b23 = BPB[23];
  uint16_t table_size_16 = b23 << 8 | b22;
  drive_mount.table_size_16 = table_size_16;

  // Get sectors per track
  uint8_t b24 = BPB[24];
  uint8_t b25 = BPB[25];
  uint16_t sectors_per_track = b25 << 8 | b24;
  drive_mount.sectors_per_track = sectors_per_track;

  // Get heads or sides on drive
  uint8_t b26 = BPB[26];
  uint8_t b27 = BPB[27];
  uint16_t head_size_count = b27 << 8 | b26;
  drive_mount.head_size_count = head_size_count;

  // Get hidden sector count
  uint8_t b28 = BPB[28];
  uint8_t b29 = BPB[29];
  uint8_t b30 = BPB[30];
  uint8_t b31 = BPB[31];
  uint32_t hidden_sector_count = b31 << 24 | b30 << 16 | b29 << 8 | b28;
  drive_mount.hidden_sector_count = hidden_sector_count;

  // Get sector count (if it didn't fit in the 16 bit entry)
  uint8_t b32 = BPB[32];
  uint8_t b33 = BPB[33];
  uint8_t b34 = BPB[34];
  uint8_t b35 = BPB[35];
  uint32_t total_sectors_32 = b35 << 24 | b34 << 16 | b33 << 8 | b32;
  drive_mount.total_sectors_32 = total_sectors_32;

  // Set FAT32 extended region values
  if (drive_mount.table_size_16 == 0) {

    // Get table size
    uint8_t b36 = BPB[36];
    uint8_t b37 = BPB[37];
    uint8_t b38 = BPB[38];
    uint8_t b39 = BPB[39];
    uint32_t table_size_32 = b39 << 24 | b38 << 16 | b37 << 8 | b36;
    extended_drive_mount_32.table_size_32 = table_size_32;

    // Get flags
    uint8_t b40 = BPB[40];
    uint8_t b41 = BPB[41];
    uint16_t flags = b41 << 8 | b40;
    extended_drive_mount_32.extended_flags = flags;

    // Get version number
    uint8_t b42 = BPB[42];
    uint8_t b43 = BPB[43];
    uint16_t fat_version = b43 << 8 | b42;
    extended_drive_mount_32.fat_version = fat_version;

    // Get cluster number of root directory
    uint8_t b44 = BPB[44];
    uint8_t b45 = BPB[45];
    uint8_t b46 = BPB[46];
    uint8_t b47 = BPB[47];
    uint32_t root_cluster = b47 << 24 | b46 << 16 | b45 << 8 | b44;
    extended_drive_mount_32.root_cluster = root_cluster;

    // Get sector number of FSinfo struct
    uint8_t b48 = BPB[48];
    uint8_t b49 = BPB[49];
    uint16_t fat_info = b49 << 8 | b48;
    extended_drive_mount_32.fat_info = fat_info;

    // Get sector number of backup boot sector
    uint8_t b50 = BPB[50];
    uint8_t b51 = BPB[51];
    uint16_t backup_BS_sector = b51 << 8 | b50;
    extended_drive_mount_32.backup_BS_sector = backup_BS_sector;

    // Get reserved sector
    memcpy(extended_drive_mount_32.reserved_0, BPB + 52, sizeof(extended_drive_mount_32.reserved_0));

    // Get drive number
    uint8_t b64 = BPB[64];
    extended_drive_mount_32.drive_number = b64;

    // Get second reserved sector (flags in Windows NT)
    uint8_t b65 = BPB[65];
    extended_drive_mount_32.reserved_1 = b65;

    // Get boot signature
    uint8_t b66 = BPB[66];
    extended_drive_mount_32.boot_signature = b66;

    // Get volume id
    uint8_t b67 = BPB[67];
    uint8_t b68 = BPB[68];
    uint8_t b69 = BPB[69];
    uint8_t b70 = BPB[70];
    uint32_t volume_id = b70 << 24 | b69 << 16 | b68 << 8 | b67;
    extended_drive_mount_32.volume_id = volume_id;

    // Get volume label string
    memcpy(extended_drive_mount_32.volume_label, BPB + 71, sizeof(extended_drive_mount_32.volume_label));

    // Get fat type label
    memcpy(extended_drive_mount_32.fat_type_label, BPB + 82, sizeof(extended_drive_mount_32.fat_type_label));
  }
  // Get extended FAT 12/16 info
  else {

    // Get bios drive number
    uint8_t b36 = BPB[36];
    extended_drive_mount_16.bios_drive_num = b36;

    // Get reserved section (flags in Windows NT)
    uint8_t b37 = BPB[37];
    extended_drive_mount_16.reserved1 = b37;

    // Get boot signature
    uint8_t b38 = BPB[38];
    extended_drive_mount_16.boot_signature = b38;

    // Get volume id serial number
    uint8_t b39 = BPB[39];
    uint8_t b40 = BPB[40];
    uint8_t b41 = BPB[41];
    uint8_t b42 = BPB[42];
    uint32_t volume_id = b42 << 24 | b41 << 16 | b40 << 8 | b39;
    extended_drive_mount_16.volume_id = volume_id;

    // Get volume lable string

    memcpy(extended_drive_mount_16.volume_label, BPB + 43, sizeof(extended_drive_mount_16.volume_label));

    // Get FAT type label
    memcpy(extended_drive_mount_16.fat_type_label, BPB + 54, sizeof(extended_drive_mount_16.fat_type_label));
  }

  // Get total sectors in volume
  uint32_t total_sectors = drive_mount.total_sectors_16 == 0 ? drive_mount.total_sectors_32 : drive_mount.total_sectors_16;
  curr_drive.total_sectors = total_sectors;

  uint32_t fat_size = drive_mount.table_size_16 == 0 ? extended_drive_mount_32.table_size_32 : drive_mount.table_size_16;
  curr_drive.fat_size = fat_size;

  uint32_t root_dir_sectors = ((drive_mount.root_entry_count * 32) + (drive_mount.bytes_per_sector - 1)) / drive_mount.bytes_per_sector;
  curr_drive.root_dir_sectors = root_dir_sectors;

  uint32_t first_data_sector = drive_mount.reserved_sector_count + (drive_mount.table_count * fat_size) + root_dir_sectors;
  curr_drive.first_data_sector = first_data_sector;

  uint32_t first_fat_sector = drive_mount.reserved_sector_count;
  curr_drive.first_fat_sector = first_fat_sector;

  uint32_t data_sectors = total_sectors - (drive_mount.reserved_sector_count + (drive_mount.table_count * fat_size) + root_dir_sectors);
  curr_drive.data_sectors = data_sectors;

  uint32_t total_clusters = data_sectors / drive_mount.sectors_per_cluster;
  curr_drive.total_clusters = total_clusters;

  uint32_t root_dir_lba = curr_drive.first_fat_sector + (drive_mount.table_count * curr_drive.fat_size) + drive_mount.hidden_sector_count;
  curr_drive.root_dir_lba = root_dir_lba;

  cwd.is_root = true;
  cwd.cluster = 0;
  cwd.name = "root";

  if (total_clusters < 4085) {
    curr_drive.fat_type = FAT12;
  } else if (total_clusters < 65535) {
    curr_drive.fat_type = FAT16;
  } else {
    curr_drive.fat_type = FAT32;
  }

  kfree(BPB);
}

bool is_end_of_entry(uint16_t cluster) {
  if (cluster >= 0xFFF7) {
    return true;
  }
  return false;
}

cwd_t *get_cwd() { return &cwd; }

uint32_t cluster_to_lba(uint16_t cluster_num) { return curr_drive.first_data_sector + ((cluster_num - 2) * drive_mount.sectors_per_cluster); }

// Given a cluster, returns the cluster number of its parent cluster
uint32_t get_parent_cluster(uint32_t cluster) {
  uint32_t curr_lba = cluster_to_lba(cluster);
  for (uint32_t i = 0; i < drive_mount.sectors_per_cluster; i++) {

    uint8_t *dir_buf = (uint8_t *)read_from_disk(curr_lba + i);

    for (int j = 0; j < drive_mount.bytes_per_sector; j += 32) {

      if (dir_buf[j] == 0) {
        kfree(dir_buf);
        goto done;
      }

      if (dir_buf[j] == 0xE5)
        continue;

      uint8_t attr = dir_buf[j + 11];

      // Skip LFN entries for now. Not supported
      if (attr == 0x0F)
        continue;

      // Is a volume_id
      if (attr == 0x08)
        continue;

      char file_name[12];
      memcpy(file_name, &dir_buf[j], 11);
      file_name[11] = '\0';
      char file_string[12];
      char *dir_name = format_from_83(file_name, file_string);
      if (strcmp(dir_name, "..")) {
        uint16_t first_cluster;
        memcpy(&first_cluster, &dir_buf[j + 26], 2);
        kfree(dir_buf);
        return first_cluster;
      }
    }
  }
done:
  printf("Parent not found\n");
  return 0;
}

// Given a cluster number, this function returns the name of the directory it belongs to.
// RETURNS HEAP ALLOCATED MEMORY
char *get_name_from_cluster(uint32_t cluster) {

  if (cluster == 0) {
    return "root";
  }

  uint32_t parent_cluster = get_parent_cluster(cluster);

  if (parent_cluster == 0) {
    for (uint32_t i = 0; i < curr_drive.root_dir_sectors; i++) {
      uint8_t *root_dir_sector_buf = (uint8_t *)read_from_disk(curr_drive.root_dir_lba + i);
      for (int j = 0; j < drive_mount.bytes_per_sector; j += 32) {

        if (root_dir_sector_buf[j] == 0) {
          kfree(root_dir_sector_buf);
          return false;
        }

        if (root_dir_sector_buf[j] == 0xE5) {
          continue;
        }

        uint8_t attr = root_dir_sector_buf[j + 11];
        if (attr == 0x0F) {
          continue;
        }

        uint16_t cluster_num;
        memcpy(&cluster_num, &(root_dir_sector_buf[j + 26]), 2);
        if (cluster_num == cluster) {
          char file_name[12];
          memcpy(file_name, &root_dir_sector_buf[j], 11);
          file_name[11] = '\0';
          char *file_string = kmalloc(12);

          format_from_83(file_name, file_string);
          kfree(root_dir_sector_buf);
          return file_string;
        }
      }
      kfree(root_dir_sector_buf);
    }
  } else {

    uint32_t curr_lba = cluster_to_lba(parent_cluster);
    for (uint32_t i = 0; i < drive_mount.sectors_per_cluster; i++) {

      uint8_t *dir_buf = (uint8_t *)read_from_disk(curr_lba + i);

      for (int j = 0; j < drive_mount.bytes_per_sector; j += 32) {

        if (dir_buf[j] == 0) {
          kfree(dir_buf);
          goto done;
        }

        if (dir_buf[j] == 0xE5)
          continue;

        uint8_t attr = dir_buf[j + 11];

        // Skip LFN entries for now. Not supported
        if (attr == 0x0F)
          continue;

        // Is a volume_id
        if (attr == 0x08)
          continue;

        // File
        if (attr == 0x20)
          continue;

        if (attr == 0x10) {
          uint32_t dir_cluster;
          memcpy(&dir_cluster, &dir_buf[j + 26], 2);

          if (dir_cluster == cluster) {
            char file_name[12];
            memcpy(file_name, &dir_buf[j], 11);
            file_name[11] = '\0';
            char *file_string = kmalloc(12);
            format_from_83(file_name, file_string);
            kfree(dir_buf);
            return file_string;
          }
        }
      }
    }
  }
done:
  return "Directory not found";
}

// Prints root directory files/directories
void read_root_dir() {
  for (uint32_t i = 0; i < curr_drive.root_dir_sectors; i++) {
    uint8_t *root_dir_sector_buf = (uint8_t *)read_from_disk(curr_drive.root_dir_lba + i);
    for (int j = 0; j < drive_mount.bytes_per_sector; j += 32) {

      if (root_dir_sector_buf[j] == 0) {
        kfree(root_dir_sector_buf);
        goto done;
      }

      if (root_dir_sector_buf[j] == 0xE5)
        continue;

      uint8_t attr = root_dir_sector_buf[j + 11];

      // Skip LFN entries for now. Not supported
      if (attr == 0x0F)
        continue;

      // Is a volume_id
      if (attr == 0x08)
        continue;

      // Directory
      if (attr == 0x10)
        terminal_setcolor(VGA_COLOR_LIGHT_BLUE);

      // File
      if (attr == 0x20)
        terminal_setcolor(VGA_COLOR_CYAN);

      char file_name[12];
      memcpy(file_name, &root_dir_sector_buf[j], 11);
      file_name[11] = '\0';
      char file_string[12];
      printf("%s\n", format_from_83(file_name, file_string));
    }

    kfree(root_dir_sector_buf);
  }
done:
  return;
}

// Prints all the data from a file given a starting cluster number
uint32_t read_file_cluster(uint32_t cluster, uint32_t bytes_remaining) {
  uint32_t curr_lba = cluster_to_lba(cluster);
  uint32_t bytes_left = bytes_remaining;
  for (uint32_t i = 0; i < drive_mount.sectors_per_cluster; i++) {
    if (bytes_left <= 0) {
      break;
    }
    uint8_t *sector_buf = (uint8_t *)read_from_disk(curr_lba + i);
    uint32_t bytes_to_print = bytes_left > 512 ? 512 : bytes_left;
    bytes_left -= bytes_to_print;

    for (uint32_t j = 0; j < bytes_to_print; j++) {
      putchar(sector_buf[j]);
    }
    kfree(sector_buf);
  }
  return bytes_left;
}

// Given a starting directory cluster, prints all entries in the directory
void read_dir_cluster(uint32_t cluster) {
  uint32_t curr_lba = cluster_to_lba(cluster);
  for (uint32_t i = 0; i < drive_mount.sectors_per_cluster; i++) {

    uint8_t *dir_buf = (uint8_t *)read_from_disk(curr_lba + i);

    for (int j = 0; j < drive_mount.bytes_per_sector; j += 32) {

      if (dir_buf[j] == 0) {
        kfree(dir_buf);
        goto done;
      }

      if (dir_buf[j] == 0xE5)
        continue;

      uint8_t attr = dir_buf[j + 11];

      // Skip LFN entries for now. Not supported
      if (attr == 0x0F)
        continue;

      // Is a volume_id
      if (attr == 0x08)
        continue;

      // Directory
      if (attr == 0x10)
        terminal_setcolor(VGA_COLOR_LIGHT_BLUE);

      // File
      if (attr == 0x20)
        terminal_setcolor(VGA_COLOR_CYAN);

      char file_name[12];
      memcpy(file_name, &dir_buf[j], 11);
      file_name[11] = '\0';
      char file_string[12];
      printf("%s\n", format_from_83(file_name, file_string));
    }
  }
done:
  return;
}

void format_into_dir_buffer(dir_entry_t *dir_entry, uint8_t *buffer, int index) {
  memcpy(&buffer[index], dir_entry->file_name, 11);
  memcpy(&buffer[index + 11], &dir_entry->file_attr, 1);
  memset(&buffer[index + 13], 0, 1);
  memset(&buffer[index + 20], 0, 2);
  memcpy(&buffer[index + 26], &dir_entry->first_cluster_num, 2);
  memset(&buffer[index + 28], 0, 4);
}

void init_dir(uint32_t parent_cluster, dir_entry_t *dir_entry) {

  dir_entry_t curr_dir;
  curr_dir.file_name = ".";
  curr_dir.file_attr = 0x10;
  curr_dir.file_size = 0;
  curr_dir.first_cluster_num = dir_entry->first_cluster_num;

  dir_entry_t parent_dir;
  parent_dir.file_name = "..";
  parent_dir.file_attr = 0x10;
  parent_dir.file_size = 0;
  parent_dir.first_cluster_num = parent_cluster;

  uint8_t *dir_buffer = kmalloc(512);

  format_into_dir_buffer(&curr_dir, dir_buffer, 0);
  format_into_dir_buffer(&parent_dir, dir_buffer, 32);

  uint32_t lba = cluster_to_lba(dir_entry->first_cluster_num);

  write_to_disk(lba, dir_buffer);
  kfree(dir_buffer);

  // init all other sectors to 0
  for (int i = 1; i < drive_mount.sectors_per_cluster; i++) {
    uint8_t *init_buffer = kmalloc(512);
    memset(init_buffer, 0, 512);
    write_to_disk(lba + i, init_buffer);
    kfree(init_buffer);
  }
}

void create_root_dir_entry(dir_entry_t *dir_entry) {
  for (uint32_t i = 0; i < curr_drive.root_dir_sectors; i++) {
    uint8_t *root_dir_sector_buf = (uint8_t *)read_from_disk(curr_drive.root_dir_lba + i);
    for (int j = 0; j < drive_mount.bytes_per_sector; j += 32) {

      uint8_t attr = root_dir_sector_buf[j + 11];

      if (attr == 0x10 || attr == 0x20) {
        char name[12];
        memcpy(name, &root_dir_sector_buf, 11);
        name[11] = '\0';
        if (strcmp(name, dir_entry->file_name)) {
          kfree(root_dir_sector_buf);
          printf("File name already exists\n");
          goto done;
        }
      }
      if (root_dir_sector_buf[j] == 0) {
        format_into_dir_buffer(dir_entry, root_dir_sector_buf, j);
        write_to_disk(curr_drive.root_dir_lba + i, root_dir_sector_buf);
        kfree(root_dir_sector_buf);

        init_dir(0, dir_entry);
        goto done;
      }
    }
  }
done:
  return;
}

void create_dir_entry(uint32_t cluster, dir_entry_t *dir_entry) {
  uint32_t curr_lba = cluster_to_lba(cluster);
  for (uint32_t i = 0; i < drive_mount.sectors_per_cluster; i++) {
    uint8_t *dir_buf = (uint8_t *)read_from_disk(curr_lba + i);
    for (int j = 0; j < drive_mount.bytes_per_sector; j += 32) {

      uint8_t attr = dir_buf[j + 11];

      if (attr == 0x10 || attr == 0x20) {
        char name[12];
        memcpy(name, &dir_buf[j], 11);
        name[11] = '\0';

        if (strcmp(name, dir_entry->file_name)) {
          printf("File name already exists\n");
          goto done;
        }
      }

      if (dir_buf[j] == 0) {
        memcpy(&dir_buf[j], dir_entry->file_name, 11);
        memcpy(&dir_buf[j + 11], &dir_entry->file_attr, 1);
        memset(&dir_buf[j + 13], 0, 1);
        memset(&dir_buf[j + 20], 0, 2);
        memcpy(&dir_buf[j + 26], &dir_entry->first_cluster_num, 2);
        memset(&dir_buf[j + 28], 0, 4);
        write_to_disk(curr_lba + i, dir_buf);
        kfree(dir_buf);

        if (dir_entry->file_attr == DIRECTORY)
          init_dir(cluster, dir_entry);

        goto done;
      }
    }
    kfree(dir_buf);
  }
done:
  return;
}

// Matches on a file or directory in the root directory
bool find_in_root(char *name, dir_entry_t *out) {
  for (uint32_t i = 0; i < curr_drive.root_dir_sectors; i++) {
    uint8_t *root_dir_sector_buf = (uint8_t *)read_from_disk(curr_drive.root_dir_lba + i);
    for (int j = 0; j < drive_mount.bytes_per_sector; j += 32) {

      if (root_dir_sector_buf[j] == 0) {
        kfree(root_dir_sector_buf);
        return false;
      }

      if (root_dir_sector_buf[j] == 0xE5) {
        continue;
      }

      uint8_t attr = root_dir_sector_buf[j + 11];
      if (attr == 0x0F) {
        continue;
      }

      char file_name[12];
      memcpy(file_name, &root_dir_sector_buf[j], 11);
      file_name[11] = '\0';
      char file_string[12];
      char *file_out = format_from_83(file_name, file_string);

      if (strcmp(file_out, name)) {
        out->file_name = name;
        out->file_attr = attr;
        memcpy(&(out->file_size), &root_dir_sector_buf[j + 28], 4);
        memcpy(&(out->first_cluster_num), &root_dir_sector_buf[j + 26], 2);
        kfree(root_dir_sector_buf);
        return true;
      }
    }
    kfree(root_dir_sector_buf);
  }
  return false;
}

// Matches on a file or directory in the current directory.
bool find_file(char *name, dir_entry_t *out) {
  if (cwd.is_root) {
    return find_in_root(name, out);
  }

  uint16_t active_cluster = cwd.cluster;
  while (!is_end_of_entry(active_cluster)) {

    uint32_t curr_lba = cluster_to_lba(active_cluster);
    for (uint32_t i = 0; i < drive_mount.sectors_per_cluster; i++) {

      uint8_t *dir_buf = (uint8_t *)read_from_disk(curr_lba + i);

      for (int j = 0; j < drive_mount.bytes_per_sector; j += 32) {

        if (dir_buf[j] == 0) {
          kfree(dir_buf);
          goto done;
        }

        if (dir_buf[j] == 0xE5)
          continue;

        uint8_t attr = dir_buf[j + 11];

        // Skip LFN entries for now. Not supported
        if (attr == 0x0F)
          continue;

        // Is a volume_id
        if (attr == 0x08)
          continue;

        char file_name[12];
        memcpy(file_name, &dir_buf[j], 11);
        file_name[11] = '\0';
        char file_string[12];
        format_from_83(file_name, file_string);

        if (strcmp(file_string, name)) {
          memcpy(&(out->first_cluster_num), &dir_buf[j + 26], 2);
          if (strcmp(name, "..")) {
            out->file_name = get_name_from_cluster(out->first_cluster_num);

          } else {
            out->file_name = name;
          }
          out->file_attr = attr;
          memcpy(&(out->file_size), &dir_buf[j + 28], 4);
          kfree(dir_buf);
          return true;
        }
      }
      kfree(dir_buf);
    }

    unsigned int fat_offset = active_cluster * 2;
    unsigned int fat_sector = curr_drive.first_fat_sector + (fat_offset / drive_mount.bytes_per_sector);
    unsigned int ent_offset = fat_offset % drive_mount.bytes_per_sector;

    uint8_t *FAT_table = (uint8_t *)read_from_disk(fat_sector);

    uint16_t table_value = *(uint16_t *)&FAT_table[ent_offset];
    active_cluster = table_value;
    kfree(FAT_table);
  }
done:
  return false;
}

// Follow a cluster chain, calling the functions that dump directory/file names to the terminal
void follow_cluster_chain(dir_entry_t entry) {
  uint16_t active_cluster = entry.first_cluster_num;
  uint32_t bytes_remaining = entry.file_size;

  while (!is_end_of_entry(active_cluster)) {
    if (entry.file_attr == 0x20) {
      read_file_cluster((uint32_t)active_cluster, bytes_remaining);
    }

    if (entry.file_attr == 0x10) {
      read_dir_cluster((uint32_t)active_cluster);
    }

    unsigned int fat_offset = active_cluster * 2;
    unsigned int fat_sector = curr_drive.first_fat_sector + (fat_offset / drive_mount.bytes_per_sector);
    unsigned int ent_offset = fat_offset % drive_mount.bytes_per_sector;

    uint8_t *FAT_table = (uint8_t *)read_from_disk(fat_sector);

    uint16_t table_value = *(uint16_t *)&FAT_table[ent_offset];
    active_cluster = table_value;
    kfree(FAT_table);
  }
}
void read_curr_dir() {
  if (cwd.is_root) {
    read_root_dir();
    return;
  }

  uint16_t active_cluster = cwd.cluster;
  while (!is_end_of_entry(active_cluster)) {
    read_dir_cluster((uint32_t)active_cluster);

    unsigned int fat_offset = active_cluster * 2;
    unsigned int fat_sector = curr_drive.first_fat_sector + (fat_offset / drive_mount.bytes_per_sector);
    unsigned int ent_offset = fat_offset % drive_mount.bytes_per_sector;

    uint8_t *FAT_table = (uint8_t *)read_from_disk(fat_sector);

    uint16_t table_value = *(uint16_t *)&FAT_table[ent_offset];
    active_cluster = table_value;
    kfree(FAT_table);
  }
}

// Set a new working directory
bool set_new_dir(char *name) {
  dir_entry_t new_entry;
  if (find_file(name, &new_entry) && new_entry.file_attr == 0x10) {
    cwd.cluster = new_entry.first_cluster_num;
    cwd.is_root = cwd.cluster == 0;
    kfree(cwd.name);
    cwd.name = new_entry.file_name;
    return true;
  }
  printf("Directory not found\n");
  return false;
}

// Iterate through file cluster chain, calling the function that dumps to terminal
void dump_file(dir_entry_t entry) {
  uint16_t active_cluster = entry.first_cluster_num;
  uint32_t bytes_remaining = entry.file_size;

  while (!is_end_of_entry(active_cluster)) {
    if (entry.file_attr != 0x20) {
      printf("Is a directory\n");
      break;
    }

    bytes_remaining = read_file_cluster((uint32_t)active_cluster, bytes_remaining);
    unsigned int fat_offset = active_cluster * 2;
    unsigned int fat_sector = curr_drive.first_fat_sector + (fat_offset / drive_mount.bytes_per_sector);
    unsigned int ent_offset = fat_offset % drive_mount.bytes_per_sector;

    uint8_t *FAT_table = (uint8_t *)read_from_disk(fat_sector);

    uint16_t table_value = *(uint16_t *)&FAT_table[ent_offset];
    active_cluster = table_value;
    kfree(FAT_table);
  }
}

uint16_t get_unused_cluster() {
  uint16_t starting_cluster = 2;
  uint16_t fat_value = 0xFFFF;

  while (true) {
    unsigned int fat_offset = starting_cluster * 2;
    unsigned int fat_sector = curr_drive.first_fat_sector + (fat_offset / drive_mount.bytes_per_sector);
    unsigned int ent_offset = fat_offset % drive_mount.bytes_per_sector;

    uint8_t *FAT_table = (uint8_t *)read_from_disk(fat_sector);
    fat_value = *(uint16_t *)&FAT_table[ent_offset];
    if (fat_value == 0x0) {
      *(uint16_t *)&FAT_table[ent_offset] = 0xFFFF;
      write_to_disk(fat_sector, FAT_table);
      kfree(FAT_table);
      break;
    }
    starting_cluster += 1;
    kfree(FAT_table);
  }

  return starting_cluster;
}

// TODO: This function needs to handle the case where last cluster is full
void create_fs_entry(char *entry_name, uint8_t attr) {
  uint16_t active_cluster = cwd.cluster;

  while (true) {
    unsigned int fat_offset = active_cluster * 2;
    unsigned int fat_sector = curr_drive.first_fat_sector + (fat_offset / drive_mount.bytes_per_sector);
    unsigned int ent_offset = fat_offset % drive_mount.bytes_per_sector;

    uint8_t *FAT_table = (uint8_t *)read_from_disk(fat_sector);

    uint16_t table_value = *(uint16_t *)&FAT_table[ent_offset];

    // We want to iterate through until we arrive at the
    // last cluster in the current directory
    if (table_value >= 0xFFF8) {
      kfree(FAT_table);
      break;
    }
    active_cluster = table_value;
    kfree(FAT_table);
  }
  dir_entry_t new_entry;
  char file_name_out[12];

  format_to_83(entry_name, file_name_out);
  new_entry.file_name = file_name_out;
  new_entry.file_attr = attr;
  new_entry.file_size = 0;
  new_entry.first_cluster_num = attr == 0x10 ? get_unused_cluster() : 0;

  if (cwd.is_root) {
    create_root_dir_entry(&new_entry);
  } else {
    create_dir_entry(active_cluster, &new_entry);
  }
}
