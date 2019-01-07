#include <driver/sd.h>

#include "utils.h"

/* Read/Write block for FAT (starts from first block of partition 1) */
u32 read_sector(u8 *buf, u32 addr, u32 count) {
    return sd_read_block(buf, addr, count);
}

u32 write_sector(u8 *buf, u32 addr, u32 count) {
    return sd_write_block(buf, addr, count);
}

u32 read_page(u32 data_start_sector, struct mem_page *crt_page) {
    return sd_read_block(crt_page->p_data, data_start_sector + crt_page->data_cluster_num * SEC_PER_CLU, SEC_PER_CLU);
}

u32 write_page(u32 data_start_sector, struct mem_page *crt_page) {
    return sd_write_block(crt_page->p_data, data_start_sector + crt_page->data_cluster_num * SEC_PER_CLU, SEC_PER_CLU);
}

/* char to u16/u32 */
u16 get_u16(u8 *ch) {
    return (*ch) + ((*(ch + 1)) << 8);
}

u32 get_u32(u8 *ch) {
    return (*ch) + ((*(ch + 1)) << 8) + ((*(ch + 2)) << 16) + ((*(ch + 3)) << 24);
}

/* u16/u32 to char */
void set_u16(u8 *ch, u16 num) {
    *ch = (u8)(num & 0xFF);
    *(ch + 1) = (u8)((num >> 8) & 0xFF);
}

void set_u32(u8 *ch, u32 num) {
    *ch = (u8)(num & 0xFF);
    *(ch + 1) = (u8)((num >> 8) & 0xFF);
    *(ch + 2) = (u8)((num >> 16) & 0xFF);
    *(ch + 3) = (u8)((num >> 24) & 0xFF);
}

/* work around */
u32 fs_wa(u32 num) {
    // return the bits of `num`
    u32 i;
    for (i = 0; num > 1; num >>= 1, i++)
        ;
    return i;
}

u32 get_first_file_name(u8 *path, u8 *name) {
    if (path[0] == '/') {
        if (path[1] == 0)
    }
}