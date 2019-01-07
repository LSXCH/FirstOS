#include <zjunix/log.h>
#include <zjunix/slab.h>

#include <zjunix/mfs/fat32cache.h>

struct D_cache *dcache;
struct P_cache *pcache;

u32 init_cache() {
    dcache = (struct D_cache*) kmalloc(sizeof(struct D_cache));
    pcache = (struct P_cache*) kmalloc(sizeof(struct P_cache));

    if (dcache == 0 || pcache == 0) {
        log(LOG_FAIL, "Cache init memory allocation fail!");
        return COMMON_ERR;
    }

    dcache->crt_size = pcache->crt_size = 0;
    dcache->max_capacity = pcache->max_capacity = C_CAPACITY;
    INIT_LIST_HEAD(&(dcache->c_LRU));
    INIT_LIST_HEAD(&(pcache->c_LRU));
    dcache->c_hashtable = (struct list_head*) kmalloc(C_TABLESIZE * sizeof(struct list_head));
    pcache->c_hashtable = (struct list_head*) kmalloc(C_TABLESIZE * sizeof(struct list_head));
    
    for (int i = 0; i < C_TABLESIZE; i++) {
        INIT_LIST_HEAD(dcache->c_hashtable+i);
        INIT_LIST_HEAD(pcache->c_hashtable+i);
    }
}

void dcache_add(struct D_cache *dcache, struct mem_dentry *data)
{
    u32 hash = __intHash(data->abs_sector_num * 16 + data->sector_dentry_offset, C_TABLESIZE);

    if (dcache->crt_size == dcache->max_capacity) {
        // dcache_drop_LRU(dcache);
    }

    list_add(&(data->d_hashlist), dcache->c_hashtable+hash);
    list_add(&(data->d_LRU), &(dcache->c_LRU))

    dcache->crt_size++;
}

u32 __intHash(u32 key, u32 size) {
    u32 mask = size - 1;
    return key & mask;
}