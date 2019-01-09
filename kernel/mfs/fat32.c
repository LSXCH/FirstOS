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
    if (path[0] != '/') {       // Wrong Path
        return 1;
    }

    struct mem_dentry *crt_directory = root_dentry;
    u32 crt_clu;
    u32 next_clu;
    u32 slash_offset = fs_cut_slash(path+slash_traverser, disk_name_str);

    // Traverse every file name in the path
    while ( slash_offset != 0 && slash_offset != 0xFFFFFFFF ) {
        if (crt_directory->dentry_data.short_attr.attr & 0x10)      // Path isn't finished, but it is't sub directory
            return 1;
        // The address in FAT block
        // Root -> 2
        crt_clu = get_clu_by_dentry(crt_directory);

        // Traverse every cluster of current direcroty
        while (crt_clu != 0x0FFFFFFF) {

            // Traverse every sector in current cluster
            for (int i = 0; i < SEC_PER_CLU; i++) {
                // Traverse every dentry in current sector
                for (int j = 0; j < DENTRY_PER_SEC; j++) {
                    int tmp_dentry_addr = total_info.data_start_sector + (crt_clu - 2) * SEC_PER_CLU + i;
                    struct mem_dentry *tmp = get_dentry(tmp_dentry_addr, j);
                    // If the first data of name attribute is 0x00, means it is the end
                    // So there is no matching file of this path
                    if (tmp->dentry_data.data[0] == 0x00)   // Not found
                        return 0;
                    else if (disk_name_cmp(tmp->dentry_data.data, disk_name_str)) {
                        crt_directory = tmp;
                        goto found_sub_dir;
                    }
                }
            }
            // Get the next cluster
            // A -> 3
            crt_clu = get_next_clu_num(crt_clu);
        }
        if (crt_clu + 2 == 0x0FFFFFFF)  // Not found
            return 0;
        // Next file name
        found_sub_dir:
        slash_traverser += slash_offset + 1;
        slash_offset = fs_cut_slash(path+slash_traverser, disk_name_str);
    }

    if (slash_offset == 0xFFFFFFFF) // Invalid path
        return 1;

    file->disk_dentry_sector_num = crt_directory->abs_sector_num;
    file->disk_dentry_num_offset = crt_directory->sector_dentry_offset;
    return 0;
}

u32 fat32_open(FILE *file, u8 *filename) {
    u32 i, j;

    kernel_memset(file->path, 0, 256);

    for (i = 0; i < 256 && filename[i] != 0; i++)
        file->path[i] = filename[i];
    // Because of memset, no need to set '\0'
    file->crt_pointer_position = 0;
    file->disk_dentry_num_offset = 0xFFFFFFFF;
    file->disk_dentry_sector_num = 0xFFFFFFFF;

    if (fat32_find(file) == 1)
        return 1;
    
    i = j = 0;
    if (filename[0] != '/') {
        while (pwd_dentry->name[i] != 0)
        {
            file->path[i] = pwd_dentry->name[i];
            i++;
        } 
        while (filename[j] != 0) {
            file->path[i+j] = filename[j];
            j++;
        }
    }

    if (file->disk_dentry_sector_num == 0xFFFFFFFF)
        return 1;
    else
        return 0;
}


u32 fat32_read(FILE *file, u8 *buf, u32 count) {
    // The clus in data field
    // root -> 2
    u32 crt_clus = get_start_cluster(file);
    u32 filesize = get_file_size(file);


    if (file->crt_pointer_position + count > filesize)
        count = filesize - file->crt_pointer_position;
    u32 start_clus_num = file->crt_pointer_position / CLUSTER_SIZE;
    u32 start_byte_num = file->crt_pointer_position % CLUSTER_SIZE;
    u32 end_clus_num = (file->crt_pointer_position + count) / CLUSTER_SIZE;
    u32 end_byte_num = (file->crt_pointer_position + count) % CLUSTER_SIZE;

    u32 clus_index = 0;
    u32 _start, _end;
    u32 buf_index = 0;
    while (crt_clus != 0x0FFFFFFF && clus_index <= end_clus_num) {
        // If it is the first cluster
        if (clus_index == start_clus_num) {
            _start = start_byte_num;
        // If it is not the first cluster, then start from 0
        } else if (clus_index > start_clus_num){
            _start = 0;
        }
        // If it is the last cluster
        if (clus_index == end_clus_num) {
            _end = end_byte_num;
        // If it is not the last cluster, then end with 4096
        } else if (clus_index < end_clus_num) {
            _end = CLUSTER_SIZE;
        }
        // If it is one of the right cluster, copy it 
        if (clus_index >= start_clus_num && clus_index <= end_clus_num) {
            struct mem_page *crt_page =  get_page(crt_clus-2);
            kernel_memcpy(buf + buf_index, crt_page->p_data+_start, _end-_start);
            buf_index += _end - _start;
        }
        // Get the next cluster num in data field
        crt_clus = get_next_clu_num(crt_clus);
        clus_index++;
    }
    // The count is already been checked
    file->crt_pointer_position += count;
}

u32 fs_write(FILE *file, const u8 *buf, u32 count) {
    if (count == 0)
        return 0;
    
    // Root -> 2
    // No data -> 0
    u32 crt_clus = get_start_cluster(file);
    u32 filesize = get_file_size(file);

    // This file has no data before
    if (crt_clus == 0) {
        
    }

    if (file->crt_pointer_position + count > filesize)
        count = filesize - file->crt_pointer_position;
    u32 start_clus_num = file->crt_pointer_position / CLUSTER_SIZE;
    u32 start_byte_num = file->crt_pointer_position % CLUSTER_SIZE;
    u32 end_clus_num = (file->crt_pointer_position + count) / CLUSTER_SIZE;
    u32 end_byte_num = (file->crt_pointer_position + count) % CLUSTER_SIZE;

    u32 clus_index = 0;
    u32 _start, _end;
    u32 buf_index = 0;
    while (clus_index <= end_clus_num) {
        // If it is the first cluster
        if (clus_index == start_clus_num) {
            _start = start_byte_num;
        // If it is not the first cluster, then start from 0
        } else if (clus_index > start_clus_num){
            _start = 0;
        }
        // If it is the last cluster
        if (clus_index == end_clus_num) {
            _end = end_byte_num;
        // If it is not the last cluster, then end with 4096
        } else if (clus_index < end_clus_num) {
            _end = CLUSTER_SIZE;
        }

        // If it is one of the right cluster, copy it 
        if (clus_index >= start_clus_num && clus_index <= end_clus_num) {
            // If it has no data before
            if (crt_clus == 0x0FFFFFFF) {

            }
        }
        // Get the next cluster num in data field
        crt_clus = get_next_clu_num(crt_clus);
        clus_index++;
    }
}

