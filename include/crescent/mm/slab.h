#pragma once

#include <crescent/types.h>
#include <crescent/core/locking.h>
#include <crescent/asm/errno.h>
#include <crescent/mm/gfp.h>

struct slab {
    void* base;
    u8* free;
    size_t in_use;
    struct slab* prev, *next;
};

struct slab_cache {
    void (*ctor)(void*);
    void (*dtor)(void*);
    struct slab* full, *partial, *empty;
    size_t obj_size;
    unsigned long obj_count;
    size_t align;
    gfp_t gfp_flags;
    spinlock_t lock;
};

/**
 * @brief Create a new slab cache
 *
 * If zero is passed to align, it will set it to 8.
 *
 * @param obj_size The size of the object. This will be rounded to the alignment
 * @param align The alignment of the object, must be a power of 2
 * @param gfp_flags The physical and virtual GFP flags for this cache
 * @param ctor Object constructor
 * @param dtor Object destructor
 *
 * @return NULL if align isn't a power of 2, or GFP_KERNEL is missing from gfp_flags,
 *              or if there's no memory available.
 */
struct slab_cache* slab_cache_create(size_t obj_size, size_t align, 
        gfp_t gfp_flags, void (*ctor)(void*), void (*dtor)(void*));

/**
 * @brief Destroy a slab cache
 * 
 * This function will make sure no slabs are empty before
 * attempting to destroy
 *
 * @param cache The cache to destroy
 *
 * @retval 0 Success
 * @retval -ECANCELLED There are still partial or full slabs
 */
int slab_cache_destroy(struct slab_cache* cache);

/**
 * @brief Create a new slab and add it into the cache
 * @param cache The cache to grow
 * @return 0 for success, -errno if an error happens
 */
int slab_cache_grow(struct slab_cache* cache);

/**
 * @brief Allocate memory from a slab cache
 *
 * This function will automatically try to grow the cache if all slabs are full. 
 *
 * @param cache The cache to allocate from
 * @return NULL if there is no memory available, otherwise you get a mapped virtual address
 */
void* slab_cache_alloc(struct slab_cache* cache);

/**
 * @brief Free memory from a slab cache
 *
 * If the object isn't in any slab, it will print an error message
 * and just return.
 *
 * @param obj The object to free
 */
void slab_cache_free(struct slab_cache* cache, void* obj);
