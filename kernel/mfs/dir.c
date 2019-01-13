#include "../../include/zjunix/mfs/dir.h"
#include "utils.h"


extern struct D_cache *dcache;
extern struct P_cache *pcache;
extern struct T_cache *tcache;

struct Total_FAT_Info total_info;
struct mem_dentry * pwd_dentry;
extern struct mem_dentry *root_dentry;

DIR * opendir(u8 *path) {
    DIR *ans = (DIR*)kmalloc(sizeof(DIR));

    if (path[0] == '/' && path[1] == 0) {
        ans->start_clus = 2;
        ans->crt_index = 0;
        return ans;
    }
    else if (path[0] == '/') {
        MY_FILE *myfile;
        fat32_open(myfile, path);
        ans->start_clus = get_start_clu_num(file);
        ans->crt_index = 0;
        return ans;
    }
}

void print_disk_name(u8 *name) {
    u8 output[20];
    int index = 0;
    for (int i = 0; i < 8; i++) {
        if (name[i] != 0x20)
            output[index++] = name[i];
    }
    if (name[8] != 0x20) {
        output[index++] = '.';
        for (int i = 8; i < 11; i++) {
            if (name[i] != 0x20)
                output[index++] = name[i];
        }
    }
    output[index] = 0;
    kernel_printf("%s\n", output);
}

dirent *readdir(DIR *dir) {
    u32 crt_clu = dir->start_clus;

    while (crt_clu != 0x0FFFFFFF) {

#ifdef FS_DEBUG
        disk_name_str[11] = 0;
        kernel_printf("The current searching is %s\n", disk_name_str);
#endif

        // Traverse every sector in current cluster
        for (u32 i = 0; i < SEC_PER_CLU; i++) {
            // Traverse every dentry in current sector
            for (u32 j = 0; j < DENTRY_PER_SEC; j++) {
                int tmp_dentry_addr = total_info.data_start_sector + (crt_clu - 2) * SEC_PER_CLU + i;
                struct mem_dentry *tmp = get_dentry(tmp_dentry_addr, j);
#ifdef FS_DEBUG
                for (int _i = 0; _i < 32; _i++)
                    kernel_printf("%c", tmp->dentry_data.data[_i]);
                kernel_printf("\n");
#endif
                // If the first data of name attribute is 0x00, means it is the end
                // So there is no matching file of this path
#ifdef FS_DEBUG
                kernel_printf("check %c.%c\n", tmp->dentry_data.data[0], tmp->dentry_data.data[8]);
#endif
                if ((*(tmp->dentry_data.data+11) & 0x08) != 0)
                {
#ifdef FS_DEBUG
                    kernel_printf("long entry\n");
#endif
                    continue;
                }
                if (tmp->dentry_data.data[0] == 0x00)   // Not found
                    return 0;
                print_disk_name(tmp->dentry_data.data);
#ifdef FS_DEBUG
                    kernel_printf("found!%d\n", 1);
                    kernel_printf("entry size %d\n", get_u32(tmp->dentry_data.data+28));
#endif
#ifdef FS_DEBUG
                else {
                    kernel_printf("Not found h.txt\n");
                }
#endif
            }
        }
        // Get the next cluster
        // A -> 3
#ifdef FS_DEBUG
        kernel_printf("crt clu:%d\n", crt_clu);
#endif
        crt_clu = get_next_clu_num(crt_clu);
#ifdef FS_DEBUG
        kernel_printf("next clu:%d\n", next_clu);
#endif
    }
}