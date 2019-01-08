#include <driver/vga.h>
#include <zjunix/log.h>
#include <zjunix/slab.h>

#include <zjunix/mfs/fat32cache.h>
#include <zjunix/mfs/fat32.h>
#include <zjunix/mfs/debug.h>

#include "utils.h"

extern struct D_cache *dcache;
extern struct P_cache *pcache;
extern struct T_cache *tcache;

struct Total_FAT_Info total_info;
struct mem_dentry * pwd_dentry;
struct mem_dentry * root_dentry;

u32 init_fat32(u32 base)
{
    if (init_total_info() == COMMON_ERR) {
        log(LOG_FAIL, "File system first step fail.");
        return COMMON_ERR;
    }

    if (init_cache() == COMMON_ERR) {
        log(LOG_FAIL, "Init file system cache fail.");
        return COMMON_ERR;
    }

    if (load_root_dentries() == COMMON_ERR) {
        log(LOG_FAIL, "Load root dentries fail.");
    }

    return 0;
}

u32 init_total_info() {
    u8 crt_buf[SECTOR_SIZE];

    //  MBR
    if (read_sector(crt_buf, 0, 1) == COMMON_ERR) {
        return COMMON_ERR;
    }
    log(LOG_OK, "MBR loaded!");
    total_info.base_addr = get_u32(crt_buf + 446 + 8);

    // BPB
    if (read_sector(total_info.bpb_info.data, total_info.base_addr, 1) == COMMON_ERR) {
        return COMMON_ERR;
    }
    log(LOG_OK, "BPB loaded!");
#ifdef FSDEBUG
    dump_bpb_info(&total_info.bpb_info.attr);
#endif

    if (total_info.bpb_info.attr.sector_size != SECTOR_SIZE) {
        log(LOG_FAIL, "FAT32 sector size must be %d bytes, but get %d bytes.", SECTOR_SIZE, total_info.bpb_info.attr.sector_size);
        return COMMON_ERR;
    }

    // Calculate the total number of data sectors for furthur using
    total_info.reserved_sectors_cnt = total_info.bpb_info.attr.reserved_sectors;
    total_info.sectors_per_FAT = total_info.bpb_info.attr.sectors_per_fat;
    total_info.data_start_sector = total_info.reserved_sectors_cnt + 2 * total_info.sectors_per_FAT;
    total_info.data_sectors_cnt = total_info.bpb_info.attr.num_of_sectors - total_info.data_start_sector;

    if (read_sector(total_info.fsi_info.data, total_info.base_addr + 1, 1) == COMMON_ERR) {
        return COMMON_ERR;
    }
    log(LOG_OK, "FSInfo loaded!");

#ifdef FSDEBUG
    data_start_sector(&total_info);
#endif

    return 0;
}

u32 load_root_dentries() {

    struct mem_page *crt_page = get_page(0);
    pwd_dentry = (struct mem_dentry *) kmalloc(sizeof(struct mem_dentry));
    root_dentry = pwd_dentry;


    pwd_dentry->name[0] = 0;
    pwd_dentry->spinned = 1;
    kernel_memcpy(pwd_dentry->dentry_data.data, crt_page->p_data, 32);
    pwd_dentry->abs_sector_num = total_info.data_start_sector;
    pwd_dentry->sector_dentry_offset = 0;
    
    dcache_add(dcache, pwd_dentry);
}

u32 fat32_find(FILE *file) {
    u8 *path = file->path;
    u8 disk_name_str[11];

    int slash_traverser = 1;
    if (path[0] != '/') {
        return 1;
    }
    struct mem_dentry *crt_directory = root_dentry;
    u32 slash_offset = fs_cut_slash(path+slash_traverser, disk_name_str);

    while ( slash_offset != 0 && slash_offset != 0xFFFFFFFF ) {
        
        for (int i = 0; i < DENTRY_PER_SEC; i++) {
            
        }
    }

    if (slash_offset == 0)      // Success
        return 0;
    else                        // Fail
        return 1;
}