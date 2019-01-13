#include <driver/vga.h>
#include <zjunix/slab.h>
#include <zjunix/log.h>

#include <zjunix/mfs/fat32cache.h>
#include <zjunix/mfs/fat32.h>
#include <zjunix/mfs/debug.h>
#include "utils.h"

extern struct Total_FAT_Info total_info;
extern struct mem_dentry * pwd_dentry;

u32 fat32_cat(u8 *path) {

    u8 filename[12];
    MY_FILE cat_file;

    /* Open */
    if (0 != fat32_open(&cat_file, path)) {
        log(LOG_FAIL, "File %s open failed", path);
        return 1;
    }

    /* Read */
    u32 file_size = get_file_size(&cat_file);
#ifdef FS_DEBUG
    kernel_printf("The file size is %d\n", file_size);
#endif 
    u8 *buf = (u8 *)kmalloc(file_size + 1);
    u8 *new_buf = (u8 *)kmalloc(2*file_size+1);

    fat32_read(&cat_file, buf, file_size);
    buf[file_size] = 0;
    kernel_printf("%s\n", buf);

    fat32_close(&cat_file);
    kfree(buf);
    return 0;
}

u32 fat32_cd(u8 *path) {
    u8 filename[12];
    MY_FILE cd_path;

    /* Open */
    if (0 != fat32_open(&cd_path, path)) {
        log(LOG_FAIL, "Path %s open failed", path);
        return 1;
    }

    struct mem_dentry *crt_entry = get_dentry(cd_path.disk_dentry_sector_num, cd_path.disk_dentry_num_offset);
    pwd_dentry->spinned = 0;
    pwd_dentry = crt_entry;
    pwd_dentry->spinned = 1;
    return 0;
}