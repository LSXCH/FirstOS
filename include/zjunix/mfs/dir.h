#ifndef MY_DIR_H
#define MY_DIR_H

#include "fat32.h"
#include "fat32cache.h"

typedef struct fs_fat32_dir {
    // in FAT block
    u32 start_clus;
    u32 crt_index;
} DIR;

typedef struct dirent_s {
    u8 *name;
} dirent;

DIR * opendir(u8 *path);
dirent *readdir(DIR *dir);

#endif