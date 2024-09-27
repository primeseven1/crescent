#pragma once

#include <crescent/types.h>
#include <crescent/asm/errno.h>
#include <crescent/mm/gfp.h>

/**
 * @brief Calculated the order of the smallest power of two number that can accommodate the size
 * @param size The size of the memory block in bytes
 * @return The order
 */
static inline unsigned int get_order(size_t size) {
    size_t block_size = 0x1000;
    unsigned int order = 0;
    
    while (block_size < size) {
        block_size <<= 1;
        order++;
    }

    return order;
}

/**
 * @brief Initialize the 3 memory zones that are used
 * 
 * This function creates the DMA, DMA32, and normal memory zones for physical memory
 * allocation. This uses a buddy system for managing physical memory.
 */
void memory_zones_init(void);

/**
 * @brief Allocates a contiguous block of physical pages from the buddy allocator
 *
 * If you want to determine the order with a size, use the get_order function to do that.
 * Any flags that the function doesn't care about are ignored. 
 *
 * If memory isn't available from a specific zone, it will try to allocate 
 * from another zone that's more restrictive than the one you want. 
 * (eg. a failed allocation from DMA32 zone will try the DMA zone, but not the normal zone).
 *
 * If you add more than 1 zone to allocate from in the flags, it will try to allocate from
 * the most restrictive zone. So just pick one zone and go with it.
 *
 * @param gfp_flags A bitmask representing the contitions for the allocation.
 *                  (valid flags: GFP_PM_ZONE_DMA, GFP_PM_ZONE_DMA32, GFP_PM_ZONE_NORMAL)
 * @param order The power of two number of contiguous pages to allocate 
 *              (eg. order 2 for 4 pages, order 1 for 2 pages, order 0 for 1 page)
 *
 * @return NULL if no pages are available, otherwise it returns a physical address to the pages
 */
void* alloc_pages(gfp_t gfp_flags, unsigned int order);

/**
 * @brief Free physical pages from the buddy allocator
 *
 * If an invalid address is passed to this function, it will just return.
 * However, an error message will be logged using printk.
 *
 * @param addr The start address of the physical pages to free
 * @param order The power of two number of contiguous pages to free
 *              (eg. order 2 for 4 pages, order 1 for 2 pages, order 0 for 1 page)
 */
void free_pages(void* addr, unsigned int order);

/**
 * @brief Allocate a single physical page from the buddy allocator
 *
 * This function is just a wrapper around alloc_pages. If you want a more extended
 * description, go to the alloc_pages function.
 *
 * @param gfp_flags A bitmask representing the conditions for the allocation
 *                  (valid flags: GFP_ZONE_DMA, GFP_ZONE_DMA32, GFP_ZONE_NORMAL)
 * @return The physical address of the page, NULL if no pages are available.
 */
static inline void* alloc_page(gfp_t gfp_flags) {
    return alloc_pages(gfp_flags, 0);
}

/**
 * @brief Free a single page from the buddy allocator
 *
 * This function is simply a wrapper around free_pages. For a more extended description,
 * read the free_pages function.
 *
 * @param addr The physical page to free
 */
static inline void free_page(void* addr) {
    free_pages(addr, 0);
}
