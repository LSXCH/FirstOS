#ifndef MY_FAT_UTILS_H
#define MY_FAT_UTILS_H

#include <zjunix/mfs/fat32.h>
#include <zjunix/mfs/debug.h>
#include <zjunix/mfs/fat32cache.h>

u32 read_sector(u8 *buf, u32 addr, u32 count);
u32 write_sector(u8 *buf, u32 addr, u32 count);
u32 read_page(struct Total_FAT_Info * total_info, struct mem_page* crt_page);
u32 write_page(struct Total_FAT_Info * total_info, struct mem_page* crt_page);
u32 read_FAT_buf(struct Total_FAT_Info * total_info, struct mem_FATbuffer* crt_buf);
u32 write_FAT_buf(struct Total_FAT_Info * total_info, struct mem_FATbuffer* crt_buf);
// Manipulate data through pointers
u16 get_u16(u8 *ch);
u32 get_u32(u8 *ch);
void set_u16(u8 *ch, u16 num);
void set_u32(u8 *ch, u32 num);
u32 fs_wa(u32 num);

// File name operations
u32 fs_cut_slash(u8 *input, u8 *name_on_disk);
u32 disk_name_cmp(u8 *a, u8 *b);
// Operations about directory entry attribute
u32 is_directory(struct mem_dentry *crt_dentry);
u32 get_clu_by_dentry(struct mem_dentry *crt_dentry);
u32 get_next_clu_num(u32 crt_clu);
u32 get_start_cluster(FILE *file);
u32 get_file_size(FILE *file);
u32 get_free_clu(u32 *output);

#endif