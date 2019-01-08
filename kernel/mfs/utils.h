#ifndef MY_FAT_UTILS_H
#define MY_FAT_UTILS_H

#include <zjunix/mfs/fat32.h>

u32 read_sector(u8 *buf, u32 addr, u32 count);
u32 write_sector(u8 *buf, u32 addr, u32 count);
u32 read_page(struct Total_FAT_Info * total_info, struct mem_page* crt_page);
u32 write_page(struct Total_FAT_Info * total_info, struct mem_page* crt_page);
// Manipulate data through pointers
u16 get_u16(u8 *ch);
u32 get_u32(u8 *ch);
void set_u16(u8 *ch, u16 num);
void set_u32(u8 *ch, u32 num);
u32 fs_wa(u32 num);
// File name operations

// Return the index of next slash or '\0'
// and put the name in between to name_on_disk
// its format is the same as the name field in dentry in disk
u32 fs_cut_slash(u8 *input, u8 *name_on_disk);


#endif