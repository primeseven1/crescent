#pragma once

#include <crescent/types.h>
#include <crescent/asm/errno.h>

enum gfp_flags {
    GFP_ZONE_DMA = (1 << 0),
    GFP_ZONE_DMA32 = (1 << 1),
    GFP_ZONE_NORMAL = (1 << 2)
};

/**
 * @brief Calculated the order of the smallest power of two number that can accommodate the size
 * @param size The size of the memory block in bytes
 * @return The order
 */
static inline unsigned int get_order(size_t size) {
    unsigned int order = 0;
    size_t pages = (size + 4096 - 1) / 4096;

    while (pages) {
        pages >>= 1;
        order++;
    }

    return order;
}

void memory_zones_init(void);

/**
 * @brief Allocates a contiguous block of physical pages
 *
 * If you want to determine the order with a size, use the get_order function to do that.
 *
 * @param gfp_flags A bitmask representing the contitions for the allocation.
 * @param order The power of two number of contiguous pages to allocate
 *
 * @return NULL if no pages are available, otherwise it returns a physical address to the pages
 */
void* alloc_pages(unsigned int gfp_flags, unsigned int order);

/**
 * @brief Free a block of allocated pages
 *
 * @param addr The start address of the physical pages to free
 * @param order The power of two number of contiguous pages to free
 */
void free_pages(void* addr, unsigned int order);

/**
 * @brief Allocate a single page
 * @param gfp_flags A bitmask representing the conditions for the allocation
 * @return A physical address to the page
 */
static inline void* alloc_page(unsigned int gfp_flags) {
    return alloc_pages(gfp_flags, 0);
}

/**
 * @brief Free a single page
 * @param addr The address to free
 */
static inline void free_page(void* addr) {
    free_pages(addr, 0);
}
