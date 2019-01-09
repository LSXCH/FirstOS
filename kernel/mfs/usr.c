#include <driver/vga.h>
#include <zjunix/slab.h>
#include <zjunix/log.h>

#include <zjunix/mfs/fat32cache.h>
#include <zjunix/mfs/fat32.h>
#include <zjunix/mfs/debug.h>
#include "utils.h"

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
    u8 *buf = (u8 *)kmalloc(file_size + 1);
    fat32_read(&cat_file, buf, file_size);
    buf[file_size] = 0;
    kernel_printf("%s\n", buf);
    // fs_close(&cat_file);
    kfree(buf);
    return 0;
}