#pragma once

#include <crescent/types.h>
#include <crescent/mm/gfp.h>

/**
 * @brief Allocate a block from the kernel heap
 *
 * @param size The size of the allocation
 * @param gfp_flags The conditions for the allocation
 *
 * @return The address of the block, NULL if there is no block
 */
void* kmalloc(size_t size, unsigned int gfp_flags);

/**
 * @brief Allocate a block of memory from the kernel heap and zero it
 *
 * This function is a wrapper around kmalloc, so read the kmalloc documentation
 * to get more info on parameters and return values.
 */
static inline void* kzalloc(size_t size, unsigned int gfp_flags) {
    return kmalloc(size, gfp_flags | GFP_ZERO);
}

/**
 * @brief Reallocate a block of memory
 *
 * This function does copy over the old block of memory to the new one.
 *
 * @param addr The address you want to reallocate
 * @param new_size The new size of the allocation
 * @param gfp_flags The new gfp flags you want to use
 *
 * @return The address of the block, NULL if there is no block
 */
void* krealloc(void* addr, size_t new_size, unsigned int gfp_flags);

/**
 * @brief Reallocate a block of memory, and zero it
 *
 * This function will zero, and then copy over the old block of memory.
 * Read the krealloc documentation for more info on this function.
 */
static inline void* kzrealloc(void* addr, size_t new_size, unsigned int gfp_flags) {
    return krealloc(addr, new_size, gfp_flags | GFP_ZERO);
}

/**
 * @brief Free a block of memory
 * @param addr The address to free
 */
void kfree(void* addr);

/**
 * @brief Initialize the ability to use a kmalloc/krealloc/kfree
 */
void heap_init(void);
