#include <crescent/common.h>
#include <crescent/mm/mmap.h>
#include <crescent/mm/slab.h>
#include <crescent/core/printk.h>
#include <crescent/lib/string.h>

/* The maximum size a slab can be before the object count goes to SLAB_AFTER_CUTOFF_COUNT */
#define SLAB_SIZE_CUTOFF 512
#define SLAB_AFTER_CUTOFF_OBJ_COUNT 16

static int slab_init(struct slab_cache* cache, struct slab* slab) {
    size_t size = cache->obj_size * cache->obj_count;
    size_t map_size = size / 8 + 1;

    /* Allocate free list bitmap */
    int errno;
    slab->free = kmmap(NULL, map_size, KMMAP_ALLOC, MMU_READ | MMU_WRITE, 
            GFP_VM_KERNEL | GFP_PM_ZONE_NORMAL, NULL, &errno);
    if (!slab->free)
        return errno;
    memset(slab->free, 0, map_size);

    /* Allocate a virtual address for the slab base */
    slab->base = kmmap(NULL, size, KMMAP_ALLOC, 
            MMU_READ | MMU_WRITE, cache->gfp_flags, NULL, &errno);
    if (!slab->base) {
        kmunmap(slab->free, map_size, KMMAP_ALLOC);
        return errno;
    }

    slab->in_use = 0;
    return 0;
}

int slab_cache_grow(struct slab_cache* cache) {
    int errno;
    /* First allocate another slab */
    struct slab* slab = kmmap(NULL, sizeof(*slab), KMMAP_ALLOC, MMU_READ | MMU_WRITE, 
            GFP_VM_KERNEL | GFP_PM_ZONE_NORMAL, NULL, &errno);
    if (!slab)
        return errno;

    /* Now initialize the slab */
    errno = slab_init(cache, slab);
    if (errno) {
        kmunmap(NULL, sizeof(*slab), KMMAP_ALLOC);
        return errno;
    }

    /* Add the slab into the list of empty slabs */
    slab->next = cache->empty;
    if (slab->next)
        slab->next->prev = slab;

    slab->prev = NULL;
    cache->empty = slab;
    return 0;
}

static void* slab_take(struct slab_cache* cache, struct slab* slab) {
    size_t obj_num = SIZE_MAX;

    /* Simply find a free block within the bitmap */
    for (size_t i = 0; i < cache->obj_count; i++) {
        size_t byte_index = i / 8;
        u8 bit_index = i % 8;

        if (!(slab->free[byte_index] & (1 << bit_index))) {
            slab->free[byte_index] |= (1 << bit_index);
            obj_num = i;
            break;
        }
    }

    if (obj_num == SIZE_MAX)
        return NULL;

    /* Now just get the object's pointer and call its constructor */
    void* obj = (u8*)slab->base + cache->obj_size * obj_num;
    if (cache->ctor)
        cache->ctor(obj);
    slab->in_use++;
    return obj;
}

static struct slab* slab_release(struct slab_cache* cache, void* obj) {
    bool partial = !cache->full;

    /* First find the slab that the block is in */
    struct slab* slab = partial ? cache->partial : cache->full;
	while (slab) {
		void* top = (void*)((uintptr_t)slab->base + cache->obj_count * cache->obj_size);
		if (obj >= slab->base && obj < top)
			break;
		if (!slab->next && !partial) {
			slab = cache->partial;
			partial = true;
		} else {
			slab = slab->next;
		}
	}

    /* This means a valid address wasn't passed to the function */
    if (!slab)
        return NULL;

    /* Get the object number and then mark it as free */
    size_t obj_num = ((uintptr_t)obj - (uintptr_t)slab->base) / cache->obj_size;
    size_t byte_index = obj_num / 8;
    size_t bit_index = obj_num % 8;
    slab->free[byte_index] &= ~(1 << bit_index);

    /* Now call the destructor and return the slab the free happened on */
    if (cache->dtor)
        cache->dtor(obj);
    slab->in_use--;
    return slab;
}

void* slab_cache_alloc(struct slab_cache* cache) {
    unsigned long flags;
    spinlock_lock_irq_save(&cache->lock, &flags);

    /* See if we have any partial or empty caches */
    struct slab* slab = NULL;
    if (cache->partial)
        slab = cache->partial;
    else if (cache->empty)
        slab = cache->empty;

    void* ret = NULL;

    /* If no slabs are available, try growing the cache */
    if (!slab) {
        if (slab_cache_grow(cache) == 0)
            slab = cache->empty;
        else
            goto out;
    }

    ret = slab_take(cache, slab);
    if (unlikely(!ret)) {
        printk(PL_CRIT "Failed to allocate slab even though the cache should have been big enough");
        goto out;
    }

    /* 
     * If the slab was empty (slab == cache->empty), move it to the partial list.
     * If the slab is now full (slab->in_use == cache->obj_count), move it to the full list.
     */
    if (slab == cache->empty) {
        cache->empty = slab->next;
        if (slab->next)
            slab->next->prev = NULL;
        if (cache->partial)
            cache->partial->prev = slab;

        slab->next = cache->partial;
        slab->prev = NULL;
        cache->partial = slab;
    } else if (slab->in_use == cache->obj_count) {
        cache->partial = slab->next;
        if (slab->next)
            slab->next->prev = NULL;
        if (cache->full)
            cache->full->prev = slab;

        slab->next = cache->full;
        slab->prev = NULL;
        cache->full = slab;
    }

out:
    spinlock_unlock_irq_restore(&cache->lock, &flags);
    return ret;
}

void slab_cache_free(struct slab_cache* cache, void* obj) {
    unsigned long flags;
    spinlock_lock_irq_save(&cache->lock, &flags);

    struct slab* slab = slab_release(cache, obj);
    if (!slab) {
        printk(PL_ERR "slab_release returned NULL, invalid object? obj: %p\n", obj);
        goto out;
    }

    /* If this happens, the slab is now empty, so move it to the empty list */
    if (slab->in_use == 0) {
		if (slab->prev == NULL)
			cache->partial = slab->next;
		else
			slab->prev->next = slab->next;

		if (slab->next)
			slab->next->prev = slab->prev;

		slab->next = cache->empty;
		slab->prev = NULL;
		if (slab->next)
			slab->next->prev = slab;
		cache->empty = slab;
	}

    /* If the slab was previously full, then move it back to the partial list */
	if (slab->in_use == cache->obj_count - 1) {
		if (slab->prev == NULL)
			cache->full = slab->next;
		else
			slab->prev->next = slab->next;

		if (slab->next)
			slab->next->prev = slab->prev;

		slab->next = cache->partial;
		slab->prev = NULL;
		if (slab->next)
			slab->next->prev = slab;
		cache->partial = slab;
	}

out:
    spinlock_unlock_irq_restore(&cache->lock, &flags);
}

struct slab_cache* slab_cache_create(size_t obj_size, size_t align, 
        gfp_t gfp_flags, void (*ctor)(void*), void (*dtor)(void*)) {
    if (gfp_flags & GFP_VM_LARGE_PAGE)
        return NULL;

    if (align == 0)
        align = 8;
    else if (align & (align - 1))
        return NULL;

    int errno;
    struct slab_cache* cache = kmmap(NULL, sizeof(*cache), KMMAP_ALLOC, 
            MMU_READ | MMU_WRITE, GFP_VM_KERNEL | GFP_PM_ZONE_NORMAL, NULL, &errno);
    if (!cache)
        return NULL;

    cache->ctor = ctor;
    cache->dtor = dtor;
    cache->full = NULL;
    cache->partial = NULL;
    cache->empty = NULL;
    cache->obj_size = ROUND_UP(obj_size, align);
    cache->obj_count = cache->obj_size < SLAB_SIZE_CUTOFF ? 0x2000 / cache->obj_size : SLAB_AFTER_CUTOFF_OBJ_COUNT;
    cache->align = align;
    cache->gfp_flags = gfp_flags;
    cache->lock = SPINLOCK_INITIALIZER;

    return cache;
}

int slab_cache_destroy(struct slab_cache* cache) {
    if (cache->partial || cache->full)
        return -ECANCELED;

    struct slab* slab = cache->empty;
	while (slab) {
		struct slab* next = slab->next;
		if (next)
    		next->prev = NULL;

        kmunmap(slab->base, cache->obj_size * cache->obj_count, KMMAP_ALLOC);
        kmunmap(slab, sizeof(*slab), KMMAP_ALLOC);

        slab = next;
    }

    kmunmap(cache, sizeof(*cache), KMMAP_ALLOC);
    return 0;
}
