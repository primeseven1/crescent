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
    void* ret = kmalloc(size, gfp_flags);
    __builtin_memset(ret, 0, size);
    return ret;
}

/**
 * @brief Reallocate a block of memory
 *
 * @param addr The address you want to reallocate
 * @param new_size The new size of the allocation
 * @param gfp_flags The new gfp flags you want to use
 *
 * @return The address of the block, NULL if there is no block
 */
void* krealloc(void* addr, size_t new_size, unsigned int gfp_flags);

/**
 * @brief Free a block of memory
 * @param addr The address to free
 */
void kfree(void* addr);
