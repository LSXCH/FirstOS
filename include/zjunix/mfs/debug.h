#ifndef _MFS_DEBUG_H
#define _MFS_DEBUG_H
#define FS_DEBUG
#ifdef FS_DEBUG
#include "fat32.h"

void dump_bpb_info_(struct BPB_attr_* bpb);
void dump_fat_info_(struct Total_FAT_Info *total_info);
void dump_page_info_(struct mem_page *page);

#endif

#endif