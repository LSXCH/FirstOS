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

struct mem_dentry * dcache_lookup(struct D_cache *dcache, u32 sector_num, u32 offset) {
    struct list_head *table_head;
    struct list_head *crt_node;
    struct mem_dentry *crt_entry;

    u32 hash = __intHash(sector_num * DENTRY_PER_SEC + offset, C_TABLESIZE);

    table_head = &(dcache->c_hashtable[hash]);

    list_for_each(crt_node, table_head) {
        crt_entry = list_entry(crt_node, struct mem_dentry, d_hashlist);
        if (crt_entry->abs_sector_num == sector_num && crt_entry->sector_dentry_offset == offset) {
            break;
        }
    }

    // Update LRU list
    if (crt_node != table_head) {
        list_del(&(crt_entry->d_LRU));
        list_add(&(crt_entry->d_LRU), &(dcache->c_LRU));
        return crt_entry;
    } else {
        return 0;
    }
}

struct mem_page * pcache_lookup(struct P_cache *pcache, u32 sector_num) {
    struct list_head *table_head;
    struct list_head *crt_node;
    struct mem_page *crt_page;
    
    u32 hash = __intHash(sector_num, C_TABLESIZE);

    table_head = &(pcache->c_hashtable[hash]);

    list_for_each(crt_node, table_head) {
        crt_page = list_entry(crt_node, struct mem_page, p_hashlist);
        if (crt_page->abs_sector_num == sector_num) {
            break;
        }
    }

    // Update LRU list
    if (crt_node != table_head) {
        list_del(&(crt_page->p_LRU));
        list_add(&(crt_page->p_LRU), &(pcache->c_LRU));
        return crt_page;
    } else {
        return 0;
    }
}

void dcache_add(struct D_cache *dcache, struct mem_dentry *data) {
    u32 hash = __intHash(data->abs_sector_num * DENTRY_PER_SEC + data->sector_dentry_offset, C_TABLESIZE);

    if (dcache->crt_size == dcache->max_capacity) {
        dcache_drop(dcache);
    }

    list_add(&(data->d_hashlist), dcache->c_hashtable+hash);
    list_add(&(data->d_LRU), &(dcache->c_LRU));

    dcache->crt_size++;
}

void pcache_add(sruct P_cache *pcache, struct mem_page *data) {
    u32 hash = __intHash(data->abs_sector_num, C_TABLESIZE);

    if (pcache->crt_size == pcache->max_capacity) {
        pcache_drop(pcache)
    }

    list_add(&(data->p_hashlist), pcache->c_hashtable+hash);
    list_add(&(data->p_LRU), &(pcache->p_LRU));

    pcache->crt_size++;
}

void dcache_drop(struct D_cache *dcache) {
    struct list_head *LRU_head = &(dcache->c_LRU);
    struct list_head *victim = LRU_head->prev;
    struct mem_dentry *crt_entry;
    
    if (dcache->crt_size == 0 || dcache->crt_size == 1) return;
    else {
        crt_entry = list_entry(victim, struct mem_dentry, d_LRU);
        if (crt_entry->is_root == 1) {
            victim = victim->prev;
            crt_entry = list_entry(victim, struct mem_dentry, d_LRU);
        }
        list_del(victim);
        list_del(&(crt_entry->d_hashlist));
        kfree(crt_entry);
        dcache->crt_size--;
    }
}

void pcache_drop(struct P_cache *pcache) {
    struct list_head *LRU_head = &(pcache->c_LRU);
    struct list_head *victim = LRU_head->prev;
    struct mem_page *crt_page;
    
    if (pcache->crt_size == 0) return;
    else {
        crt_page = list_entry(victim, struct mem_page, p_LRU);
        list_del(victim);
        list_del(&(crt_page->p_hashlist));
        kfree(crt_page->p_data);
        pcache->crt_size--;
    }
}

u32 __intHash(u32 key, u32 size) {
    u32 mask = size - 1;
    return key & mask;
}