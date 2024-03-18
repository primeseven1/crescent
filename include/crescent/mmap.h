#pragma once

#include <crescent/asm/multiboot2.h>

/*
 * Get the total amount of bytes of free memory 
 * 
 * --- Notes ---
 * - The result may be a few megabytes less than expected, either due to reserved memory,
 * or memory mapped regions
 *
 * --- Return values ---
 * - 0: Call store_mbi_mmap
 * - Any other value: Success
 */
size_t get_total_mmap_free_memory(void);

/*
 * Check if a physical address range is free according to the memory map
 *
 * --- Parameters ---
 * - paddr: Physical address
 * - size: How many bytes you want to check
 *
 * --- Return values ---
 * - true: Self explanitory
 * - false: Returned if paddr is not found, or if store_mbi_mmap hasn't been called yet
 */
bool is_paddr_mmap_region_free(void* paddr, size_t size);

/*
 * Get a memory map entry
 *
 * --- Notes ---
 * - Do not assume that just because it's free in the entry means it's actually free
 * use is_paddr_mmap_region free to verify that, it checks for overlapping memory regions
 *
 * --- Parameters ---
 * - index: The index of the memory map entry 
 *
 * --- Return values ---
 * - NULL: Index is either invalid or out of range, or store_mbi_mmap hasn't been called
 * - The entry: Success
 */
const struct multiboot_mmap_entry* get_mmap_entry(unsigned int index);

/* 
 * Store the multiboot memory map so the memory management part of the kernel can use it
 *
 * Notes:
 * This should only be called once
 */
void store_mbi_mmap(const struct multiboot_tag_mmap* mmap_tag);
