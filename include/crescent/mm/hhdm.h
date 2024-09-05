#pragma once

#include <crescent/core/limine.h>

/**
 * @brief Get the physical address of a virtual address with limine's HHDM feature
 * @return The physical address
 */
static inline void* hhdm_physical(void* addr) {
    return (u8*)addr - g_limine_hhdm_request.response->offset;
}

/**
 * @brief Get the virtual address of a physical address with limine's HHDM feature
 * @return The virtual address
 */
static inline void* hhdm_virtual(void* addr) {
    return (u8*)addr + g_limine_hhdm_request.response->offset;
}
