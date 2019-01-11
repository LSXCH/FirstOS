#include <zjunix/mfs/debug.h>
#ifdef FS_DEBUG

#include <driver/vga.h>
void dump_bpb_info_(struct BPB_attr_* bpb) {
    kernel_printf("BPB size: %d\n", sizeof(struct BPB_attr_));
    kernel_printf("Sector size: %x\n", bpb->sector_size);
}

void dump_fat_info_(struct Total_FAT_Info *total_info) {
    kernel_printf("BaseAddress: %d\n", total_info->base_addr);
    kernel_printf("Reserved sectors: %d\n", total_info->reserved_sectors_cnt);
    kernel_printf("Data sectors count: %d\n", total_info->data_sectors_cnt);
    kernel_printf("Sectors per FAT: %d\n", total_info->sectors_per_FAT);
    kernel_printf("Data start sector: %d\n", total_info->data_start_sector);
}

void dump_page_info_(struct mem_page *page) {
    kernel_printf("This page is dirty ? %d\n", page->state);
    kernel_printf("The cluster number in data field is %d\n", page->data_cluster_num);
}

#endif
