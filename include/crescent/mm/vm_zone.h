#pragma once

#include <crescent/types.h>
#include <crescent/mm/gfp.h>

/**
 * @brief Allocate virtual pages
 *
 * This function does not map the virtual page to memory. This simply allocates a free
 * virtual address.
 *
 * @param gfp_flags A bitmask for the conditions of the allocation. valid flags start with GFP_VM
 * @param order The power of two number of contiguous pages to allocate 
 *              (eg. order 2 for 4 pages, order 1 for 2 pages, order 0 for 1 page)
 *
 * @return NULL if no pages are available, otherwise it returns the virtual address
 */
void* alloc_vpages(unsigned int gfp_flags, unsigned int order);

/**
 * @brief Reserve virtual pages
 *
 * If any error happens, no pages get reserved.
 *
 * @param addr The address to reserve
 * @param order The power of two number of contiguous pages to allocate 
 *              (eg. order 2 for 4 pages, order 1 for 2 pages, order 0 for 1 page) 
 *
 * @retval -EOVERFLOW The address + page_count is outside of the virtual memory zone
 * @retval -EADDRNOTAVAIL No zone available for the allocation
 * @retval -EADDRINUSE The address range that you want has all or some of the pages already allocated
 * @retval 0 Success
 */
int reserve_vpages(void* addr, unsigned int order);

/**
 * @brief Free virtual pages
 *
 * @param addr The address to free
 * @param order The power of two number of contiguous pages to free
 *              (eg. order 2 for 4 pages, order 1 for 2 pages, order 0 for 1 page)
 */
void free_vpages(void* addr, unsigned int order);

/**
 * @brief Allocate a single virtual page
 *
 * This function is a wrapper around alloc_vpages
 *
 * @param gfp_flags A bitmask for the conditions of the allocation. valid flags start with GFP_VM
 * @param NULL if no page is available, otherwise it returns the pointer to the virtual page
 */
static inline void* alloc_vpage(unsigned int gfp_flags) {
    return alloc_vpages(gfp_flags, 0);
}

/**
 * @brief Free a virtual page
 *
 * This function is a wrapper around free_vpages
 *
 * @param addr The address to free
 */
static inline void free_vpage(void* addr) {
    free_vpages(addr, 0);
}

/**
 * @brief Initialize the ability to use virtual memory zones
 */
void vm_zone_init(void);
