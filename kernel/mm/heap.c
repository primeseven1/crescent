#include <crescent/common.h>
#include <crescent/core/printk.h>
#include <crescent/core/panic.h>
#include <crescent/core/cpu.h>
#include <crescent/mm/mmap.h>
#include <crescent/mm/heap.h>
#include <crescent/mm/slab.h>
#include <crescent/lib/string.h>

#define HEAP_CHK_VALUE 0xdecafc0ffee
#define HEAP_CACHE_COUNT 12

static const size_t heap_cache_sizes[HEAP_CACHE_COUNT] = { 
    256, 256, 256, 512, 512, 512, 1024, 1024, 1024, 2048, 2048, 2048 
};
static struct slab_cache* heap_caches[HEAP_CACHE_COUNT] = { NULL };
static const size_t alloc_info_size = sizeof(size_t) * 3;

static struct slab_cache* get_slab_cache(size_t size, unsigned int gfp_flags) {
    struct slab_cache* cache = NULL;
    for (size_t i = 0; i < ARRAY_SIZE(heap_caches); i++) {
        if (size <= heap_caches[i]->obj_size && heap_caches[i]->gfp_flags == gfp_flags) {
            cache = heap_caches[i];
            break;
        }
    }
    return cache;
}

void* kmalloc(size_t size, unsigned int gfp_flags) {
    bool zero = gfp_flags & GFP_ZERO;
    gfp_flags &= ~GFP_ZERO;

    size = ROUND_UP(size, 8); 
    struct slab_cache* cache = get_slab_cache(size + alloc_info_size, gfp_flags);

    /* If there is no slab that can accomodate the size or gfp flags, it will just call kmmap directly */
    size_t* alloc_info;
    if (cache) {
        alloc_info = slab_cache_alloc(cache);
        if (!alloc_info)
            return NULL;
    } else {
        int errno;
        alloc_info = kmmap(NULL, size + alloc_info_size, 
                MMU_FLAG_READ_WRITE | MMU_FLAG_NX, gfp_flags, &errno);
        if (alloc_info == (void*)-1)
            return NULL;
    }

    /* Now store the allocation info */
    alloc_info[0] = size;
    alloc_info[1] = gfp_flags;

    u8* ret = (u8*)(alloc_info + 2);
    if (zero)
        memset(ret, 0, size);

    size_t* check_value = (size_t*)(ret + size);
    *check_value = HEAP_CHK_VALUE;

    return ret;
}

void* krealloc(void* addr, size_t new_size, unsigned int gfp_flags) {
    bool zero = gfp_flags & GFP_ZERO;
    gfp_flags &= ~GFP_ZERO;

    new_size = ROUND_UP(new_size, 8);
    size_t* old_alloc_info = (size_t*)addr - 2;

    size_t old_size = old_alloc_info[0];
    size_t old_gfp_flags = old_alloc_info[1];

    size_t* check_value = (size_t*)((u8*)addr + old_size);
    if (*check_value != HEAP_CHK_VALUE)
        panic("Kernel heap corruption! Check value: %zu", *check_value);

    struct slab_cache* new_cache = get_slab_cache(new_size + alloc_info_size, gfp_flags);
   
    /* Like in kmalloc, try to allocate from a slab cache, But if that's not possible, use kmmap */
    size_t* new_alloc_info;
    if (new_cache) {
        new_alloc_info = slab_cache_alloc(new_cache);
        if (!new_alloc_info)
            return NULL;
    } else {
        int errno;
        new_alloc_info = kmmap(NULL, new_size + alloc_info_size,
                MMU_FLAG_READ_WRITE | MMU_FLAG_NX, gfp_flags, &errno);
        if (new_alloc_info == (void*)-1)
            return NULL;
    }

    /* Now copy over memory from the old location */
    u8* ret = (u8*)(new_alloc_info + 2);
    if (new_size > old_size) {
        if (zero)
            memset(ret, 0, new_size);
        memcpy(ret, addr, old_size);
    } else {
        memcpy(ret, addr, new_size);
    }

    check_value = (size_t*)((u8*)ret + new_size);
    *check_value = HEAP_CHK_VALUE;

    /* If old_cache is NULL, then kmmap was used to allocate the block */
    struct slab_cache* old_cache = get_slab_cache(old_size, old_gfp_flags);
    if (old_cache) {
        slab_cache_free(old_cache, old_alloc_info);
    } else {
        int errno = kmunmap(old_alloc_info, old_size + alloc_info_size, old_gfp_flags, true);
        if (unlikely(errno))
            printk(PL_CRIT "There was an error unmapping the heap memory (addr %p), how did this happen? (err: %i)\n", 
                    addr, errno);
    }

    return ret;
}

void kfree(void* addr) {
    size_t* alloc_info = (size_t*)addr - 2;

    size_t alloc_size = alloc_info[0];
    size_t alloc_gfp_flags = alloc_info[1];

    size_t* check_value = (size_t*)((u8*)addr + alloc_size);
    if (*check_value != HEAP_CHK_VALUE)
        panic("Kernel heap corruption! Check value: %zu", *check_value);

    /* If cache is NULL, kmmap was used to allocate the block */
    struct slab_cache* cache = get_slab_cache(alloc_size + alloc_info_size, alloc_gfp_flags);
    if (cache) {
        slab_cache_free(cache, alloc_info);
    } else {
        int err = kmunmap(alloc_info, alloc_size + alloc_info_size, alloc_gfp_flags, true);
        if (unlikely(err))
            printk(PL_CRIT "There was an error unmapping the heap memory (addr %p), how did this happen? (err: %i)\n", 
                    addr, err);
    }
}

void heap_init(void) {
    for (size_t i = 0; i < ARRAY_SIZE(heap_caches); i++) {
        unsigned int gfp_zone;
        if (i % 3 == 0)
            gfp_zone = GFP_PM_ZONE_NORMAL;
        else if (i % 3 == 1)
            gfp_zone = GFP_PM_ZONE_DMA32;
        else
            gfp_zone = GFP_PM_ZONE_DMA;

        struct slab_cache* cache = slab_cache_create(heap_cache_sizes[i], 8,
                GFP_VM_KERNEL | gfp_zone, NULL, NULL);
        if (!cache)
            panic("Failed to allocate slab caches for heap!\n");
        heap_caches[i] = cache;
    }
}
