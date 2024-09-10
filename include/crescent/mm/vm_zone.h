#pragma once

#include <crescent/types.h>
#include <crescent/core/locking.h>
#include <crescent/mm/zone.h>

struct vm_zone {
    void* base;
    unsigned long page_count;
    unsigned int flags;
    u8* map;
    spinlock_t lock; /* Only needed for shared memory mappings */
};

/**
 * @brief Allocate virtual pages
 *
 * This function will NOT map the page, this simply gives you an available virtual
 * address.
 *
 * @param gfp_flags A bitmask for the conditions of the allocation
 * @param order The power of two number of contiguous pages to allocate 
 *              (eg. order 2 for 4 pages, order 1 for 2 pages, order 0 for 1 page)
 * 
 * @return NULL if a virtual address cannot be returned. Otherwise it returns a pointer.
 */
void* alloc_pages_virtual(unsigned int gfp_flags, unsigned int order);

/**
 * @brief Free virtual pages
 *
 * This function will NOT unmap the page, this just marks the address free in the
 * allocator.
 *
 * @param addr The address to free
 * @param order The power of two number of contiguous pages to allocate 
 *              (eg. order 2 for 4 pages, order 1 for 2 pages, order 0 for 1 page)
 */
void free_pages_virtual(void* addr, unsigned int order);

/**
 * @brief Allocate a single virtual page
 * @param gfp_flags A bitmask for the conditions of the allocation
 * @return NULL is no virtual address can be returned. Otherwise it returns a pointer.
 */
static inline void* alloc_page_virtual(unsigned int gfp_flags) {
    return alloc_pages_virtual(gfp_flags, 0);
}

/**
 * @brief Free a single page
 * @param addr The address to free
 */
static inline void free_page_virtual(void* addr) {
    free_pages_virtual(addr, 0);
}

/**
 * @brief Initialize the ability to use virtual memory zones
 */
void vm_zone_init(void);
