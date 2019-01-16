#include <zjunix/log.h>
#include <driver/vga.h>
#include <zjunix/slab.h>
#include <zjunix/utils.h>

#include <zjunix/mfs/fat32cache.h>
#include <zjunix/mfs/debug.h>

#include "utils.h"
#include "../fs/fat/utils.h"

struct D_cache *dcache;
struct P_cache *pcache;
struct T_cache *tcache;

extern struct Total_FAT_Info total_info;

u32 init_cache() {
    // allocate memory
    dcache = (struct D_cache*) kmalloc(sizeof(struct D_cache));
    pcache = (struct P_cache*) kmalloc(sizeof(struct P_cache));
    tcache = (struct T_cache*) kmalloc(sizeof(struct T_cache));

    if (dcache == 0 || pcache == 0 || tcache == 0) {
        log(LOG_FAIL, "Cache init memory allocation fail!");
        return COMMON_ERR;
    }

    // init attributes
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
    // look up first
    struct mem_dentry * result = dcache_lookup(dcache, sector_num, offset);
    // if not found
    if (result == 0) {
        result = (struct mem_dentry *) kmalloc(sizeof(struct mem_dentry));
        result->spinned = 0;
        result->abs_sector_num = sector_num;
        result->sector_dentry_offset = offset;
        u32 page_cluster_num = (sector_num - total_info.data_start_sector) / SEC_PER_CLU;
//#ifdef FS_DEBUG
        // kernel_printf("query page %d\n", page_cluster_num);
        // kernel_printf("offset = %d\n", offset);
//#endif
        // require the corresponding page
        struct mem_page * location_page = get_page(page_cluster_num);
        kernel_memcpy(result->dentry_data.data, location_page->p_data + offset * DENTRY_SIZE, DENTRY_SIZE);
        dcache_add(dcache, result);
        return result;
    } else {
        kernel_printf("dcache look up found!\n");
        return result;
    }
}

// Input the cluster to data field
struct mem_page * get_page(u32 relative_cluster_num) {
    // look up first
    struct mem_page * result = pcache_lookup(pcache, relative_cluster_num);
    // if not found
    if (result == 0) {
        result = (struct mem_page *) kmalloc(sizeof(struct mem_page));
        result->state = PAGE_CLEAN;
        result->data_cluster_num = relative_cluster_num;
        result->p_data = (u8 *) kmalloc(CLUSTER_SIZE);
        // read the corresponding page on disk
        read_page(&total_info, result);
        pcache_add(pcache, result);
#ifdef FSDEBUG
        dump_page_info(result);
#endif
        return result;
    }
    return result;
}

struct mem_FATbuffer *get_FATBuf(u32 FAT_num, u32 sec_num) {
    // look up first then same as page cache
    struct mem_FATbuffer * result = tcache_lookup(tcache, FAT_num, sec_num);

    if (result == 0) {
        result = (struct mem_FATbuffer *) kmalloc(sizeof(struct mem_FATbuffer));
        result->state = PAGE_CLEAN;
        result->fat_num = FAT_num;
        result->sec_num_in_FAT = sec_num;
        result->t_data = (u8 *) kmalloc(SECTOR_SIZE);
#ifdef FS_DEBUG
        kernel_printf("malloced address t_data : %x\n", result->t_data);
#endif
        read_FAT_buf(&total_info, result);
#ifdef FS_DEBUG
        kernel_printf("BEFORE TACHE ADD!\n");
#endif
        tcache_add(tcache, result);
        return result;
    }
    
    return result;
}

struct mem_dentry * dcache_lookup(struct D_cache *dcache, u32 sector_num, u32 offset) {
    struct list_head *table_head;
    struct list_head *crt_node;
    struct mem_dentry *crt_entry;

    // Get hash value
    u32 hash = __intHash(sector_num * DENTRY_PER_SEC + offset, C_TABLESIZE);

    // get hash list head
    table_head = dcache->c_hashtable + hash;
    // kernel_printf("hash head index is %x\n", table_head);
    list_for_each(crt_node, table_head) {
        // kernel_printf("just test %x\n", crt_node);
        crt_entry = list_entry(crt_node, struct mem_dentry, d_hashlist);
        // check if they match
        if (crt_entry->abs_sector_num == sector_num && crt_entry->sector_dentry_offset == offset) {
            break;
        }
    }

    // Update LRU list
    if (crt_node != table_head) {
        // delete and add can make it to the head
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

    // Get hash value
    u32 hash = __intHash(relative_cluster_num, C_TABLESIZE);

    // get hash list head
    table_head = pcache->c_hashtable + hash;

    list_for_each(crt_node, table_head) {
        crt_page = list_entry(crt_node, struct mem_page, p_hashlist);
        // check if they match
        if (crt_page->data_cluster_num == relative_cluster_num) {
#ifdef FS_DEBUG
            kernel_printf("look up found!%d\n", relative_cluster_num);
#endif
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

    // Get hash value
    u32 hash = __intHash(sec_num, C_TABLESIZE);

    // get hash list head
    table_head = tcache->c_hashtable+hash;

    list_for_each(crt_node, table_head) {
        crt_buf = list_entry(crt_node, struct mem_FATbuffer, t_hashlist);
        // check if they match
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

    // If full, drop one
    if (dcache->crt_size == dcache->max_capacity) {
        dcache_drop(dcache);
    }


    // link current node to hash list and LRU list
    list_add(&(data->d_hashlist), &(dcache->c_hashtable[hash]));
    list_add(&(data->d_LRU), &(dcache->c_LRU));

    dcache->crt_size++;
}

// The same as above
void pcache_add(struct P_cache *pcache, struct mem_page *data) {
    u32 hash = __intHash(data->data_cluster_num, C_TABLESIZE);

    if (pcache->crt_size == pcache->max_capacity) {
        pcache_drop(pcache);
    }

    list_add(&(data->p_hashlist), &(pcache->c_hashtable[hash]));
    list_add(&(data->p_LRU), &(pcache->c_LRU));

    pcache->crt_size++;
}

// The same as above
void tcache_add(struct T_cache *tcache, struct mem_FATbuffer *data) {
    u32 hash = __intHash(data->sec_num_in_FAT, C_TABLESIZE);

    if (tcache->crt_size == tcache->max_capacity) {
        tcache_drop(tcache);
    }

    list_add(&(data->t_hashlist), &(tcache->c_hashtable[hash]));
    list_add(&(data->t_LRU), &(tcache->c_LRU));
#ifdef FS_DEBUG
    kernel_printf("TCACHE ADD: added!");
#endif
}

// Drop one item
void dcache_drop(struct D_cache *dcache) {
#ifdef FS_DEBUG
    kernel_printf("DROP DCACHE\n");
#endif
    struct list_head *LRU_head = &(dcache->c_LRU);
    struct list_head *victim = LRU_head->prev;
    struct mem_dentry *crt_entry;
    
    // If no cache or just root cache, return 
    if (dcache->crt_size == 0 || dcache->crt_size == 1) return;
    else {
        crt_entry = list_entry(victim, struct mem_dentry, d_LRU);
        // check whether spinned
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
#ifdef FS_DEBUG
    kernel_printf("DROP PCACHE\n");
#endif
    struct list_head *LRU_head = &(pcache->c_LRU);
    struct list_head *victim = LRU_head->prev;
    struct mem_page *crt_page;
    
    // if it has no pcache reutrn 
    if (pcache->crt_size == 0) return;
    else {
        // delete from LRU and hash list
        crt_page = list_entry(victim, struct mem_page, p_LRU);
        list_del(victim);
        list_del(&(crt_page->p_hashlist));

        // if it is dirty, write back
        if (crt_page->state == PAGE_DIRTY) {
            write_page(&total_info, crt_page);
        }
        kfree(crt_page->p_data);
        kfree(crt_page);
        pcache->crt_size--;
    }
}

// The same as above
void tcache_drop(struct T_cache *tcache) {
#ifdef FS_DEBUG
    kernel_printf("DROP PCACHE\n");
#endif
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

// Use address to updagte FAT
u32 update_FAT(u32 crt_clus, u32 next_clus) {
    // get FAT buf by cluster
    struct mem_FATbuffer *crtBuffer = get_FATBuf(1, crt_clus * 4 / SECTOR_SIZE);
    set_u32(crtBuffer->t_data + crt_clus * 4, next_clus);
    crtBuffer->state = PAGE_DIRTY;
    // update fat table
    crtBuffer = get_FATBuf(2, crt_clus * 4 / SECTOR_SIZE);
    set_u32(crtBuffer->t_data + crt_clus * 4, next_clus);
    crtBuffer->state = PAGE_DIRTY;
    
    return 0;
}

// Flush all the cache to sd card
void fat32_fflush() {
    int dsize = dcache->crt_size;
    int psize = pcache->crt_size;
    int tsize = tcache->crt_size;
    for (int i = 0; i < dsize; i++) {
        dcache_drop(dcache);
    }
    for (int i = 0; i < psize; i++) {
        pcache_drop(pcache);
    }
    for (int i = 0; i < tsize; i++) {
        tcache_drop(tcache);
    }
}

// Get hash value
u32 __intHash(u32 key, u32 size) {
    u32 mask = size - 1;
    return key & mask;
}