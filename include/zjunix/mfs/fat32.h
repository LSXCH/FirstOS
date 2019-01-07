#ifndef MY_FAT32_H
#define MY_FAT32_H
#include <zjunix/type.h>
#include <zjunix/list.h>

#include <zjunix/mfs/debug.h>

#include "errno.h"

#define SECTOR_SIZE 512
#define CLUSTER_SIZE 4096
#define SEC_PER_CLU 8
#define DENTRY_PER_SEC 16

struct __attribute__((__packed__)) disk_short_dentry_addr {
    u8 name[8];                   /* Name */
    u8 ext[3];                    /* Extension */
    u8 attr;                      /* attribute bits */
    u8 lcase;                     /* Case for base and extension */
    u8 ctime_cs;                  /* Creation time, centiseconds (0-199) */
    u16 ctime;                    /* Creation time */
    u16 cdate;                    /* Creation date */
    u16 adate;                    /* Last access date */
    u16 starthi;                  /* Start cluster (Hight 16 bits) */
    u16 time;                     /* Last modify time */
    u16 date;                     /* Last modify date */
    u16 startlow;                 /* Start cluster (Low 16 bits) */
    u32 size;                     /* file size (in bytes) */
};

struct __attribute__((__packed)) disk_long_dentry_addr {
    u8 L_ord;
    u8 name1[10];
    u8 L_addr;
    u8 L_type;
    u8 L_chksum;
    u8 name2[12];
    u16 no_mean;
    u8 name3[4];
};

union disk_dentry {
    u8 data[32];
    struct disk_short_dentry_addr short_attr;
    // struct disk_long_dentry_addr  long_addr;
};

typedef struct fat_file_s {
    // The absolute path
    u8 path[256];
    // The current file pointer
    u32 crt_pointer_position;
    // The sector number of the directory entry on disk
    u32 disk_dentry_sector_num;
    u32 disk_dentry_num_in;
} FILE;

struct __attribute__((__packed__)) BPB_attr {
    // 0x00 ~ 0x0f
    u8 jump_code[3];
    u8 oem_name[8];
    u16 sector_size;
    u8 sectors_per_cluster;
    u16 reserved_sectors;
    // 0x10 ~ 0x1f
    u8 number_of_copies_of_fat;
    u16 max_root_dir_entries;
    u16 num_of_small_sectors;
    u8 media_descriptor;
    u16 sectors_per_fat;
    u16 sectors_per_track;
    u16 num_of_heads;
    u32 num_of_hidden_sectors;
    // 0x20 ~ 0x2f
    u32 num_of_sectors;
    u32 num_of_sectors_per_fat;
    u16 flags;
    u16 version;
    u32 cluster_number_of_root_dir;
    // 0x30 ~ 0x3f
    u16 sector_number_of_fs_info;
    u16 sector_number_of_backup_boot;
    u8 reserved_data[12];
    // 0x40 ~ 0x51
    u8 logical_drive_number;
    u8 unused;
    u8 extended_signature;
    u32 serial_number;
    u8 volume_name[11];
    // 0x52 ~ 0x1fe
    u8 fat_name[8];
    u8 exec_code[420];
    u8 boot_record_signature[2];
};

union BPB_Info {
    u8 data[SECTOR_SIZE];
    struct BPB_attr attr;
};

struct __attribute__((__packed)) FSI_attr {
    u32 lead_signature;
    u8  reserved1[480];
    u32 structure_signature;
    u32 free_clustor_count;
    u32 next_free_cluster;
    u8  reserved2[12];
    u32 trail_signature;
};

union FSI_Info {
    u8 data[SECTOR_SIZE];
    struct FSI_attr attr;
};

struct Total_FAT_Info {
    u32 base_addr;
    u32 reserved_sectors_cnt;
    u32 data_sectors_cnt;
    u32 sectors_per_FAT;
    u32 data_start_sector;
    union BPB_Info bpb_info;
    union FSI_Info fsi_info;
};

struct mem_dentry {
    u8 is_root;
    u8 path_name[256];
    u32 abs_sector_num;
    u32 sector_dentry_offset;
    union disk_dentry dentry_data;
    struct list_head d_hashlist;
    struct list_head d_LRU;
};

struct mem_page {

    u8 *p_data;
    u8 state;
    u32 abs_sector_num;
    struct list_head p_hashlist;
    struct list_head p_LRU;
};

u32 init_fat32();
u32 init_total_info();
u32 load_root_dentries();

#endif