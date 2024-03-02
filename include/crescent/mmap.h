#pragma once

#include <crescent/asm/multiboot2.h>

/*
 * Get the total amount of bytes of free memory 
 * 
 * Notes:
 * The result may be a few megabytes less than expected, either due to reserved memory,
 * or memory mapped regions
 *
 * If this returns 0, make sure you have called store_mbi_mmap
 */
size_t get_total_mmap_free_memory(void);

/*
 * Check if a physical address range is free according to the memory map
 *
 * Return values:
 * true: Self explanitory
 * false: Returned if paddr is not found, or if store_mbi_mmap hasn't been called yet
 */
bool is_paddr_mmap_region_free(void* paddr, size_t size);

/* 
 * Store the multiboot memory map so the memory management part of the kernel can use it
 *
 * Notes:
 * This should only be called once
 */
void store_mbi_mmap(const struct multiboot_tag_mmap* mmap_tag);
