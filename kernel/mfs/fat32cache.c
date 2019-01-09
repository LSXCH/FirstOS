#include <zjunix/log.h>
#include <zjunix/slab.h>

#include <zjunix/mfs/fat32cache.h>
#include <zjunix/mfs/debug.h>

#include "utils.h"
#include "../fs/fat/utils.h"

struct D_cache *dcache;
struct P_cache *pcache;
struct T_cache *tcache;

extern struct Total_FAT_Info total_info;

u32 init_cache() {
    dcache = (struct D_cache*) kmalloc(sizeof(struct D_cache));
    pcache = (struct P_cache*) kmalloc(sizeof(struct P_cache));
    tcache = (struct T_cache*) kmalloc(sizeof(struct T_cache));

    if (dcache == 0 || pcache == 0 || tcache == 0) {
        log(LOG_FAIL, "Cache init memory allocation fail!");
        return COMMON_ERR;
    }

    dcache->crt_size = pcache->crt_size = tcache->crt_size = 0;
    dcache->max_capacity = pcache->max_capacity = tcache->max_capacity = C_CAPACITY;
    INIT_LIST_HEAD(&(dcache->c_LRU));
    INIT_LIST_HEAD(&(pcache->c_LRU));
    INIT_LIST_HEAD(&(tcache->c_LRU));
    dcache->c_hashtable = (struct list_head*) kmalloc(C_TABLESIZE * sizeof(struct list_head));
    pcache->c_hashtable = (struct list_head*) kmalloc(C_TABLESIZE * sizeof(struct list_head));
    tcache->c_hashtable = (struct list_head*) kmalloc(C_TABLESIZE * sizeof(struct list_head));
    
    for (int i = 0; i < C_TABLESIZE; i++) {
        INIT_LIST_HEAD(dcache->c_hashtable+i);
        INIT_LIST_HEAD(pcache->c_hashtable+i);
        INIT_LIST_HEAD(tcache->c_hashtable+i);
    }
}


// Return the mem_dentry struct with no path name
struct mem_dentry * get_dentry(u32 sector_num, u32 offset) {
    struct mem_dentry * result = dcache_lookup(dcache, sector_num, offset);
    if (result == 0) {
        result = (struct mem_dentry *) kmalloc(sizeof(struct mem_dentry));
        result->spinned = 0;
        result->abs_sector_num = sector_num;
        result->sector_dentry_offset = offset;
        u32 page_cluster_num = (sector_num - total_info.data_start_sector) / SEC_PER_CLU;
        struct mem_page * location_page = get_page(page_cluster_num);
        kernel_memcpy(result->dentry_data.data, location_page->p_data + offset * DENTRY_SIZE, DENTRY_SIZE);
        dcache_add(dcache, result);
    } else {
        return result;
    }
}

// Input the cluster to data field
struct mem_page * get_page(u32 relative_cluster_num) {
    struct mem_page * result = pcache_lookup(pcache, relative_cluster_num);

    if (result == 0) {
        result = (struct mem_page *) kmalloc(sizeof(struct mem_page));
        result->state = PAGE_CLEAN;
        result->data_cluster_num = relative_cluster_num;
        result->p_data = (u8 *) kmalloc(CLUSTER_SIZE);
        read_page(&total_info, result);
        pcache_add(pcache, result);
#ifdef FSDEBUG
        dump_page_info(result);
#endif
    }
    return result;
}

struct mem_FATbuffer *get_FATBuf(u32 FAT_num, u32 sec_num) {
    struct mem_FATbuffer * result = tcache_lookup(tcache, FAT_num, sec_num);

    if (result == 0) {
        result = (struct mem_FATbuffer *) kmalloc(sizeof(struct mem_FATbuffer));
        result->state = PAGE_CLEAN;
        result->fat_num = FAT_num;
        result->sec_num_in_FAT = sec_num;
        result->t_data = (u8 *) kmalloc(SECTOR_SIZE);
        read_FAT_buf(&total_info, result);
        tcache_add(tcache, result);
    }
    
    return result;
}

struct mem_dentry * dcache_lookup(struct D_cache *dcache, u32 sector_num, u32 offset) {
    struct list_head *table_head;
    struct list_head *crt_node;
    struct mem_dentry *crt_entry;

    u32 hash = __intHash(sector_num * DENTRY_PER_SEC + offset, C_TABLESIZE);

    table_head = dcache->c_hashtable + hash;

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

struct mem_page * pcache_lookup(struct P_cache *pcache, u32 relative_cluster_num) {
    struct list_head *table_head;
    struct list_head *crt_node;
    struct mem_page *crt_page;
    
    u32 hash = __intHash(relative_cluster_num, C_TABLESIZE);

    table_head = pcache->c_hashtable + hash;

    list_for_each(crt_node, table_head) {
        crt_page = list_entry(crt_node, struct mem_page, p_hashlist);
        if (crt_page->data_cluster_num == relative_cluster_num) {
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

struct mem_FATbuffer * tcache_lookup(struct T_cache *tcache, u32 FAT_num, u32 sec_num) {
    struct list_head *table_head;
    struct list_head *crt_node;
    struct mem_FATbuffer *crt_buf;

    u32 hash = __intHash(sec_num, C_TABLESIZE);

    table_head = tcache->c_hashtable+hash;

    list_for_each(crt_node, table_head) {
        crt_buf = list_entry(crt_node, struct mem_FATbuffer, t_hashlist);
        if (crt_buf->fat_num == FAT_num && crt_buf->sec_num_in_FAT == sec_num) {
            break;
        }
    }

    // Update LRU list
    if (crt_node != table_head) {
        list_del(&(crt_buf->t_LRU));
        list_add(&(crt_buf->t_LRU), &(tcache->c_LRU));
        return crt_buf;
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

void pcache_add(struct P_cache *pcache, struct mem_page *data) {
    u32 hash = __intHash(data->data_cluster_num, C_TABLESIZE);

    if (pcache->crt_size == pcache->max_capacity) {
        pcache_drop(pcache);
    }

    list_add(&(data->p_hashlist), pcache->c_hashtable+hash);
    list_add(&(data->p_LRU), &(pcache->c_LRU));

    pcache->crt_size++;
}

void tcache_add(struct T_cache *tcache, struct mem_FATbuffer *data) {
    u32 hash = __intHash(data->sec_num_in_FAT, C_TABLESIZE);

    if (tcache->crt_size == tcache->max_capacity) {
        tcache_drop(tcache);
    }

    list_add(&(data->t_hashlist), tcache->c_hashtable+hash);
    list_add(&(data->t_LRU), &(tcache->c_LRU));
}

void dcache_drop(struct D_cache *dcache) {
    struct list_head *LRU_head = &(dcache->c_LRU);
    struct list_head *victim = LRU_head->prev;
    struct mem_dentry *crt_entry;
    
    if (dcache->crt_size == 0 || dcache->crt_size == 1) return;
    else {
        crt_entry = list_entry(victim, struct mem_dentry, d_LRU);
        if (crt_entry->spinned != 0) {
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

        if (crt_page->state == PAGE_DIRTY) {
            write_page(&total_info, crt_page);
        }
        kfree(crt_page->p_data);
        kfree(crt_page);
        pcache->crt_size--;
    }
}

void tcache_drop(struct T_cache *tcache) {
    struct list_head *LRU_head = &(tcache->c_LRU);
    struct list_head *victim = LRU_head->prev;
    struct mem_FATbuffer *crt_buf;

    if (tcache->crt_size == 0) return;
    else {
        crt_buf = list_entry(victim, struct mem_FATbuffer, t_LRU);
        list_del(victim);
        list_del(&(crt_buf->t_hashlist));

        if (crt_buf->state == PAGE_DIRTY) {
            write_FAT_buf(&total_info, crt_buf);
        }
        kfree(crt_buf->t_data);
        kfree(crt_buf);
        tcache->crt_size--;
    }
}

u32 update_FAT(u32 crt_clus, u32 next_clus) {
    struct mem_FATbuffer *crtBuffer = get_FATBuf(1, crt_clus * 4 / SECTOR_SIZE);
    set_u32(crtBuffer->t_data + crt_clus * 4, next_clus);
    crtBuffer->state = PAGE_DIRTY;
    crtBuffer = get_FATBuf(2, crt_clus * 4 / SECTOR_SIZE);
    set_u32(crtBuffer->t_data + crt_clus * 4, next_clus);
    crtBuffer->state = PAGE_DIRTY;
    
    return 0;
}

void fat32_fflush() {
    for (int i = 0; i < dcache->crt_size; i++) {
        dcache_drop(dcache);
    }
    for (int i = 0; i < pcache->crt_size; i++) {
        pcache_drop(pcache);
    }
    for (int i = 0; i < tcache->crt_size; i++) {
        tcache_drop(tcache);
    }
}


u32 __intHash(u32 key, u32 size) {
    u32 mask = size - 1;
    return key & mask;
}