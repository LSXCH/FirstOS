#ifndef MY_FAT32CACHE_H
#define MY_FAT32CACHE_H

#include <zjunix/type.h>
#include <zjunix/list.h>

#include "fat32.h"

#define C_CAPACITY  16
#define C_TABLESIZE 32

struct D_cache {
    u32 max_capacity;
    u32 crt_size;
    u32 table_size;
    struct list_head c_LRU;
    struct list_head *c_hashtable;
};

struct P_cache {
    u32 max_capacity;
    u32 crt_size;
    u32 table_size;
    struct list_head c_LRU;
    struct list_head *c_hashtable;
};

u32 init_cache();
struct mem_dentry * dcache_lookup(struct D_cache *dcache, u32 sector_num, u32 offset);
struct mem_page * pcache_lookup(struct P_cache *pcache, struct mem_page *data);
void dcache_add(struct D_cache *dcache, struct mem_dentry *data);
void pcache_add(struct P_cache *pcache, struct mem_page* data);
void dcache_drop(struct D_cache *dcache);
void pcache_drop(struct P_cache *pcache);

u32 __intHash(u32 key, u32 size);

#endif