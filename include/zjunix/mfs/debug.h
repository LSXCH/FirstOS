#ifndef _MFS_DEBUG_H
#define _MFS_DEBUG_H

#ifdef FS_DEBUG
#include "fat32.h"

void dump_bpb_info(struct BPB_attr* bpb);
void dump_fat_info(struct Total_FAT_Info *total_info);
void dump_page_info(struct mem_page *page);

#endif

#endif