#include <driver/sd.h>

#include "utils.h"
#include "../fs/fat/utils.h"

struct Total_FAT_Info total_info;

/* Read/Write block for FAT (starts from first block of partition 1) */
u32 read_sector(u8 *buf, u32 addr, u32 count) {
    return sd_read_block(buf, addr, count);
}

u32 write_sector(u8 *buf, u32 addr, u32 count) {
    return sd_write_block(buf, addr, count);
}

u32 read_page(struct Total_FAT_Info * total_info, struct mem_page* crt_page) {
    u32 sector_address = total_info->base_addr + total_info->data_start_sector + crt_page->data_cluster_num * SEC_PER_CLU;
    return sd_read_block(crt_page->p_data, sector_address, SEC_PER_CLU);
}

u32 write_page(struct Total_FAT_Info * total_info, struct mem_page* crt_page) {
    u32 sector_address = total_info->base_addr + total_info->data_start_sector + crt_page->data_cluster_num * SEC_PER_CLU;
    return sd_write_block(crt_page->p_data, sector_address, SEC_PER_CLU);
}


u32 read_FAT_buf(struct Total_FAT_Info * total_info, struct mem_FATbuffer* crt_buf) {
    u32 sector_address = total_info->base_addr + total_info->reserved_sectors_cnt + 
                        total_info->sectors_per_FAT * (crt_buf->fat_num-1) + crt_buf->sec_num_in_FAT;
    return sd_read_block(crt_buf->t_data, sector_address, 1);
}
u32 write_FAT_buf(struct Total_FAT_Info * total_info, struct mem_FATbuffer* crt_buf) {
    u32 sector_address = total_info->base_addr + total_info->reserved_sectors_cnt + 
                        total_info->sectors_per_FAT * (crt_buf->fat_num-1) + crt_buf->sec_num_in_FAT;
    return sd_write_block(crt_buf->t_data, sector_address, 1);
}

// /* char to u16/u32 */
// u16 get_u16(u8 *ch) {
//     return (*ch) + ((*(ch + 1)) << 8);
// }

// u32 get_u32(u8 *ch) {
//     return (*ch) + ((*(ch + 1)) << 8) + ((*(ch + 2)) << 16) + ((*(ch + 3)) << 24);
// }

// /* u16/u32 to char */
// void set_u16(u8 *ch, u16 num) {
//     *ch = (u8)(num & 0xFF);
//     *(ch + 1) = (u8)((num >> 8) & 0xFF);
// }

// void set_u32(u8 *ch, u32 num) {
//     *ch = (u8)(num & 0xFF);
//     *(ch + 1) = (u8)((num >> 8) & 0xFF);
//     *(ch + 2) = (u8)((num >> 16) & 0xFF);
//     *(ch + 3) = (u8)((num >> 24) & 0xFF);
// }

// /* work around */
// u32 fs_wa(u32 num) {
//     // return the bits of `num`
//     u32 i;
//     for (i = 0; num > 1; num >>= 1, i++)
//         ;
//     return i;
// }

// Return the index of next slash or '\0'
// and put the name in between to name_on_disk
// its format is the same as the name field in dentry in disk
u32 fs_cut_slash(u8 *input, u8 *name_on_disk) {
    int slash_index, dot_index;

    // Get the end of the input
    for (slash_index = 0; input[slash_index] != 0 && input [slash_index] != '/'; slash_index++) ;
    // If it is the end of the path
    if (slash_index == 0) return 0;
    
    // Init name_on_disk
    for (int i = 0; i < 11; i++) name_on_disk[i] = 0x20;
    // If it is . or ..
    if (slash_index == 1 && input[0] == '.') {
         name_on_disk[0] = '.';
    }
    else if (slash_index == 2 && input[0] == '.' && input[1] == '.') {
        name_on_disk[0] = '.';
        name_on_disk[1] = '.';
    } else if (input[0] == '.') {
        return 0xFFFFFFFF;
    }

    // Get the first dot index
    for (dot_index = 0; input[dot_index] != '.' && dot_index < slash_index; dot_index++) ;
    for (int i = 0; i < dot_index && i < 8; i++) {
        
        if (input[i] == 0x22 || input[i] == 0x2A || input[i] >= 0x2B && input[i] <= 0x2F || 
            input[i] >= 0x3A && input[i] <= 0x3F || input[i] >= 0x5B && input[i] <= 0x5D ||
            input[i] >= 0x7C && input[i] <= 0x7E ) {
            return 0xFFFFFFFF;
        } else {
            name_on_disk[i] = input[i];
            if (input[i] >= 'a' && input[i] <= 'z')
                name_on_disk[i] = input[i] - 'a' + 'A';
        }
    }
    for (int i = 1; dot_index + i < slash_index && i <= 3; i++) {
        u8 ch = input[dot_index + i];
        if (ch >= 'a' && ch <= 'z')
            name_on_disk[7+i] = ch - 'a' + 'A';
        else if (ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9')
            name_on_disk[7+i] = ch;
        else
            return 0xFFFFFFF;
    }

    return slash_index;
}
u32 disk_name_cmp(u8 *a, u8 *b) {
    for (int i = 0; i < 11; i++)
        if (a[i] != b[i]) return 1;

    return 0;
}

inline u32 is_directory(struct mem_dentry *crt_dentry) {
    u8 attr = crt_dentry->dentry_data.short_attr.attr;
    return (attr & 0x08) || (attr & 0x10);
}

// Get the index of FAT
// For example: 0FFFFFF8 FFFFFFFF 0FFFFFFF 0FFFFFFF
// data:   / : 00 00 return 2
// data:   A : 03 00 return 3
u32 get_clu_by_dentry(struct mem_dentry *crt_dentry) {
    
    // If it is the root directory
    if (crt_dentry->dentry_data.short_attr.attr & 0x08)
        return 2;

    u32 clu_num = (u32)(crt_dentry->dentry_data.short_attr.starthi) << 16 | crt_dentry->dentry_data.short_attr.startlow;
#ifdef FS_DEBUG
    u32 hi = get_u16(&crt_dentry->dentry_data.short_attr.starthi);
    u32 lo = get_u16(&crt_dentry->dentry_data.short_attr.startlo);
    u32 debug = (u32)hi << 16 | lo;
    kernel_printf("FS_DEBUG   direct : %d, debug : %d\n", clu_num, debug);
#endif
    return clu_num;
}

// Input  : The index of FAT
//          root -> 2
// Output : The num in FAT block
//          FAT[2] -> 3
u32 get_next_clu_num(u32 crt_clu) {
    struct mem_FATbuffer *crt_FAT = get_FATBuf(1, crt_clu / DENTRY_PER_SEC);
    return get_u32(crt_FAT->t_data + crt_clu * 4) & 0x0FFFFFFF;
}

// Get the index of FAT
// file:root
// return 2
u32 get_start_clu_num(MY_FILE *file) {
    struct mem_dentry *crt_entry = get_dentry(file->disk_dentry_sector_num, file->disk_dentry_num_offset);
    return get_clu_by_dentry(crt_entry);
}

// Input a FILE struct with disk_dentry_sector_num
// and disk_dentry_num_offset initialized
// Return the file size
u32 get_file_size(MY_FILE *file) {
    struct mem_dentry *crt_entry = get_dentry(file->disk_dentry_sector_num, file->disk_dentry_num_offset);
    return crt_entry->dentry_data.short_attr.size;
}

// Input the pointer to output
// The free cluster num is saved in output
// The cluster number in data field
u32 get_free_clu(u32 *output) {
    // Get the cluster which is allocated latestly
    u32 start_clu = total_info.fsi_info.attr.next_free_cluster;
    if (start_clu == 0xFFFFFFFF)
        start_clu = 2;
    
    for (int i = start_clu; i < total_info.sectors_per_FAT * SECSIZE / 4; i++) {
        // if ()
    }
}