#ifndef MY_FAT_UTILS_H
#define MY_FAT_UTILS_H

#include <zjunix/mfs/fat32.h>

u32 read_sector(u8 *buf, u32 addr, u32 count);
u32 write_sector(u8 *buf, u32 addr, u32 count);
u32 read_page(u32 data_start_sector, struct mem_page* crt_page);
u32 write_page(u32 data_start_sector, struct mem_page* crt_page);
// Manipulate data through pointers
u16 get_u16(u8 *ch);
u32 get_u32(u8 *ch);
void set_u16(u8 *ch, u16 num);
void set_u32(u8 *ch, u32 num);
u32 fs_wa(u32 num);
// File name operations

u32 get_first_file_name(u8 *path, u8 *name);

u32 check_all_cap(u8 *name);

#endif