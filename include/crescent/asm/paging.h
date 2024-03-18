#pragma once

#define PAGE_SIZE 4096

#define KERNEL_VMA_OFFSET 0xFFFFFF8000000000

#ifdef __ASSEMBLY_FILE__

#define V2P(a) ((a) - KERNEL_VMA_OFFSET)
#define P2V(a) ((a) + KERNEL_VMA_OFFSET)

#else /* __ASSEMBLY_FILE__ */

#include <crescent/kernel.h>
#include <crescent/types.h>

#define V2P(a) ((void*)((uintptr_t)(a) - KERNEL_VMA_OFFSET))
#define P2V(a) ((void*)((uintptr_t)(a) + KERNEL_VMA_OFFSET))

/* Align addresses based on how much memory you can map with a single entry */
#define PAGE_DIRECTORY_POINTER_ALIGN(a) ((void*)((uintptr_t)(a) / 0x8000000000 * 0x8000000000))
#define PAGE_DIRECTORY_ALIGN(a) ((void*)((uintptr_t)(a) / 0x40000000 * 0x40000000))
#define PAGE_TABLE_ALIGN(a) ((void*)((uintptr_t)(a) / 0x200000 * 0x200000))
#define PAGE_ALIGN(a) ((void*)((uintptr_t)(a) / PAGE_SIZE * PAGE_SIZE))

typedef u64 generic_pte_t;

typedef u64 pml4e_t;
typedef u64 pdpte_t;
typedef u64 pde_t;
typedef u64 pte_t;

enum pt_levels {
    PT_LEVEL_PAGE,
    PT_LEVEL_PT,
    PT_LEVEL_PD,
    PT_LEVEL_PDPT,
    PT_LEVEL_PML4
};

enum pt_flags {
    PT_PRESENT = (1 << 0),
    PT_READ_WRITE = (1 << 1),
    PT_USER_SUPERVISOR = (1 << 2),
    PT_WRITETHROUGH = (1 << 3),
    PT_CACHE_DISABLE = (1 << 4),
    PT_ACCESSED = (1 << 5),
    PT_UNUSED6 = (1 << 6),
    PT_HUGE_PAGE = (1 << 7),
    PT_UNUSED8 = (1 << 8),
    PT_UNUSED9 = (1 << 9),
    PT_UNUSED10 = (1 << 10),
    PT_UNUSED11 = (1 << 11),
    PT_EXECUTE_DISABLE = ((u64)1 << 63)
};

/*
 * Flush a single entry in the TLB
 *
 * Paramaters:
 * - vaddr: Virtual address
 */
static inline void tlb_flush_single(void* addr)
{
    asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

/* Flush the entire TLB */
static inline void tlb_flush_all(void)
{
    asm volatile("movq %%cr3, %%rax\n"
                 "movq %%rax, %%cr3"
                 :
                 :
                 : "rax", "memory");
}

/*
 * Get the physical address that vaddr maps to
 *
 * --- Parameters ---
 * - vaddr: The virtual address
 *
 * --- Return values ---
 * - Returns NULL if the page is unmapped, otherwise it returns the physical address
 */
void* get_paddr(const void* vaddr);

/*
 * Map a page table
 *
 * --- Parameters ---
 * - vaddr: The virtual address, must be canonical
 * - pt_paddr: The physical address of the table, must be aligned by 4K
 * - flags: Page table flags, PT_PRESENT and PT_WRITABLE are automatically added
 * - level: The page table level, look at the pt_levels enum for valid values
 *
 * --- Return values ---
 * - -E2BIG: pt_paddr has a bigger address than the CPU supports
 * - -EINVAL: Invalid flags, non-canonical vaddr, or pt_paddr is not aligned
 * - -EBUSY: Page table is already present, or the PML4 index is the recursive page table entry
 * - -EADDRNOTAVAIL: PML4/PDPT/PD entries are not present, depends on the level of the page table
 * - 0: Success
 */
int map_page_table(void* vaddr, const generic_pte_t* pt_paddr, u64 flags, unsigned int level);

/*
 * Unmap a page table
 * 
 * --- Parameters ---
 * - vaddr: The virtual address
 * - level: The page table level, look at the pt_levels enum for more info
 *
 * --- Return values ---
 * - -EINVAL: vaddr is non-canonical, or level is invalid
 * - -EADDRNOTAVAIL: pml4/pdpt/pd entries are not present
 * - -EBUSY: vaddr uses the recursive PML4 entry
 * - 0: Success
 */
int unmap_page_table(void* vaddr, unsigned int level);

/*
 * Map a page into memory
 * 
 * --- Parameters ---
 * - vaddr: The virtual address, automatically aligned
 * - paddr: The physical address
 * - flags: Page flags
 *
 * --- Return values ---
 * - -E2BIG: paddr is bigger than the amount the CPU supports
 * - -EINVAL: Invalid flags, or vaddr is non-canonical
 * - -EADDRNOTAVAIL: pml4/pdpt/pd entries are not present
 * - -EBUSY: Page table entry already present, or the vaddr uses the recursive PML4 entry
 * - 0: Success
 */
int map_page(void* vaddr, const void* paddr, u64 flags);

/*
 * Unmap a page
 *
 * --- Parameters ---
 * - vaddr: Virtual address, automatically aligned
 *
 * --- Return values ---
 * - -EINVAL: vaddr is non-canonical
 * - -EADDRNOTAVAIL: pml4/pdpt/pd entries are not present
 * - -EBUSY: vaddr uses the recursive PML4 entry
 * - 0: Success
 */
int unmap_page(void* vaddr);

/*
 * Map several pages
 *
 * --- Notes ---
 * - This unmaps all pages that were previously mapped if an error happens
 *
 * --- Parameters ---
 * - vaddr: Virtual address
 * - paddr: Physical address
 * - flags: Page flags
 * - count: Number of pages
 *
 * --- Return values ---
 * - The exact same as map_page
 */
int map_pages(void* vaddr, const void* paddr, u64 flags, size_t count);

/*
 * Unmap several pages
 *
 * --- Parameters ---
 * - vaddr: Virtual address
 * - count: Number of pages
 *
 * --- Return values ---
 * - The exact same as unmap_page
 */
int unmap_pages(void* vaddr, size_t count);

#endif /* __ASSEMBLY_FILE__ */
