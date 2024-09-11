#pragma once

#include <crescent/types.h>
#include <crescent/mm/gfp.h>

void* alloc_pages_virtual(unsigned int gfp_flags, unsigned int order);

void free_pages_virtual(void* addr, unsigned int order);

static inline void* alloc_page_virtual(unsigned int gfp_flags) {
    return alloc_pages_virtual(gfp_flags, 0);
}

static inline void free_page_virtual(void* addr) {
    free_pages_virtual(addr, 0);
}

/**
 * @brief Initialize the ability to use virtual memory zones
 */
void vm_zone_init(void);
