#include <zjunix/mfs/debug.h>
#include <zjunix/mfs/
#ifdef FS_DEBUG

#include <driver/vgs.h>
void dump_bpb_info(struct BPB_attr* bpb) {
    kernel_printf("BPB size: %d\n", sizeof(struct BPB_attr));
    kernel_printf("Sector size: %x\n", BPB->sector_size);
}

void dump_fat_info(struct Total_FAT_Info *total_info) {
    kernel_printf("BaseAddress: %x\n", total_info->base_addr);
    kernel_printf("Reserved sectors: %x\n", total_info->reserved_sectors_cnt);
    kernel_printf("Data sectors count: %x\n", total_info->data_sectors_cnt);
    kernel_printf("Sectors per FAT: %x\n", total_info->sectors_per_FAT);
    kernel_printf("Data start sector: %x\n", total_info->data_start_sector);
}

void dump_page_info(struct mem_page *page) {
    kernel_printf("This page is dirty ? %d\n", page->state);
    kernel_printf("The cluster number in data field is %d\n", page->data_cluster_num);
}

#endif
