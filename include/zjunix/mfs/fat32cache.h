#ifndef MY_FAT32CACHE_H
#define MY_FAT32CACHE_H

#include <zjunix/type.h>
#include <zjunix/list.h>

#include "fat32.h"

#define C_CAPACITY  16
#define C_TABLESIZE 32

#define update_dentry(__sec_num, __offset, __attr, __val) { \
    struct mem_dentry * tmp_dentry = get_dentry(__sec_num, __offset); \
    u32 page_cluster_num = (__sec_num - total_info.data_start_sector) / SEC_PER_CLU; \
    struct mem_page * tmp_page = get_page(page_cluster_num); \
    \
    tmp_dentry->dentry_data.short_attr.__attr = __val; \
    union disk_dentry *attributes = (union disk_dentry *)tmp_page->p_data + __offset * DENTRY_SIZE; \
    attributes->short_attr.__attr = __val; \
    tmp_page->state = PAGE_DIRTY; \
}

struct arguments {
    void *arg1;
    void *arg2;
};


struct redis_cache {
    u32 max_capacity;
    u32 crt_size;
    u32 table_size;
    struct list_head *c_hashtable;
};

struct redis_cache_oprations {
    void* (*get)(struct redis_cache*, struct arguments*);
    void* (*look_up)(struct redis_cache*, struct arguments*);
    void (*add)(struct redis_cache*, void*);
    void (*drop_entry)(void*);
};


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

struct T_cache {
    u32 max_capacity;
    u32 crt_size;
    u32 table_size;
    struct list_head c_LRU;
    struct list_head *c_hashtable;
};

u32 init_cache();

struct mem_dentry * get_dentry(u32 sector_num, u32 offset);
struct mem_page * get_page(u32 relative_cluster_num);
struct mem_FATbuffer *get_FATBuf(u32 FAT_num, u32 sec_num);

struct mem_dentry * dcache_lookup(struct D_cache *dcache, u32 sector_num, u32 offset);
struct mem_page * pcache_lookup(struct P_cache *pcache, u32 relative_cluster_num);
struct mem_FATbuffer * tcache_lookup(struct T_cache *tcache, u32 FAT_num, u32 sec_num);
void dcache_add(struct D_cache *dcache, struct mem_dentry *data);
void pcache_add(struct P_cache *pcache, struct mem_page* data);
void tcache_add(struct T_cache *tcache, struct mem_FATbuffer *data);
void dcache_drop(struct D_cache *dcache);
void pcache_drop(struct P_cache *pcache);
void tcache_drop(struct T_cache *tcache);

u32 update_FAT(u32 crt_clus, u32 next_clus);

void fat32_fflush();

u32 __intHash(u32 key, u32 size);

#endif