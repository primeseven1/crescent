#pragma once

#include <crescent/asm/multiboot2.h>

/*
 * Get the total amount of bytes of free memory
 * If this returns 0, call store_mbi_mmap
 */
size_t get_total_free_memory(void);

/* 
 * Store the multiboot memory map so the memory management part of the kernel can use it,
 * should only be called once
 */
void store_mbi_mmap(const struct multiboot_tag_mmap* mmap_tag);
