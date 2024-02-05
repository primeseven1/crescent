#pragma once

#define PAGE_SIZE 4096
#define PAGE_TABLE_SIZE 4096
#define PAGE_DIRECTORY_SIZE 4096
#define PAGE_TABLE_ALIGN 4096
#define PAGE_DIRECTORY_ALIGN 4096

#define KERNEL_VMA_OFFSET 0xC0000000

#ifndef __ASSEMBLY_FILE__

#include <crescent/types.h>
#include <crescent/error.h>

#define V2P(a) ((void*)((uintptr_t)(a) - KERNEL_VMA_OFFSET))
#define P2V(a) ((void*)((uintptr_t)(a) + KERNEL_VMA_OFFSET))

#define ALIGN_TO_PAGE(a) ((void*)((uintptr_t)(a) / PAGE_SIZE * PAGE_SIZE))

typedef u32 pte_t;

enum page_directory_flags {
    PD_PRESENT = (1 << 0),
    PD_READ_WRITE = (1 << 1),
    PD_USER_SUPERVISOR = (1 << 2),
    PD_WRITETHROUGH = (1 << 3),
    PD_CACHE_DISABLE = (1 << 4),
    PD_ACCESSED = (1 << 5),
    PD_HUGE_PAGE = (1 << 7)
};

enum page_table_flags {
    PT_PRESENT = (1 << 0),
    PT_READ_WRITE = (1 << 1),
    PT_USER_SUPERVISOR = (1 << 2),
    PT_WRITETHROUGH = (1 << 3),
    PT_CACHE_DISABLE = (1 << 4),
    PT_ACCESSED = (1 << 5),
    PT_DIRTY = (1 << 6),
    PT_ATTRIBUTE_TABLE = (1 << 7),
    PT_GLOBAL = (1 << 8)
};

/*
 * vaddr - Virtual address
 * paddr - Physical address to map vaddr to
 * flags - flags for the page table entry, look at the page_directory_flags enum for more values
 * Return values:
 * -ERR_PDE_NOT_PRESENT: Since there is no heap allocator, or any dynamic allocation (yet)
 *  so this can only map memory when the page directory entry already exists
 * -ERR_PTE_PRESENT: Returned when the PTE entry for the address you're trying to map is present
 * -ERR_MISALIGNED_ADDR: Returned when either vaddr or paddr are not aligned by a page boundary (4K)
 */
int map_page(const void* vaddr, const void* paddr, int flags);

/*
 * vaddr - Virtual address
 * Return values:
 * -ERR_MISALIGNED_ADDR: Returned when vaddr is not aligned by a page boundary (4K)
 */
int unmap_page(const void* vaddr);

/*
 * Notes: On error, this will unmap every page that it successfully mapped before the error happened
 * vaddr - Virtual address
 * paddr - Physical address to map vaddr to
 * flags - flags for the page table entry, look at page_directory_flags for more info
 * count - Number of pages
 * Return values: Same as the map_page function
 */
int map_pages(const void* vaddr, const void* paddr, int flags, size_t count);

/*
 * vaddr - Virtal address
 * count - Number of pages to unmap
 * Return values: Same as the unmap_page function
 */
int unmap_pages(const void* vaddr, size_t count);

#else

#define V2P(a) (a - KERNEL_VMA_OFFSET)
#define P2V(a) (a + KERNEL_VMA_OFFSET)

#endif /* __ASSEMBLY_FILE__ */
