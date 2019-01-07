#include <zjunix/mfs/debug.h>

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

#endif
