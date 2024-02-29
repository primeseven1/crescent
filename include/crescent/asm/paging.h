#pragma once

#define PAGE_SIZE 4096

#define KERNEL_VMA_OFFSET 0xFFFFFF8000000000

#ifdef __ASSEMBLY_FILE__

#define V2P(a) ((a) - KERNEL_VMA_OFFSET)
#define P2V(a) ((a) + KERNEL_VMA_OFFSET)

#else

#include <crescent/kernel.h>
#include <crescent/types.h>

#define V2P(a) (void*)((uintptr_t)(a) - KERNEL_VMA_OFFSET)
#define P2V(a) (void*)((uintptr_t)(a) + KERNEL_VMA_OFFSET)
#define PAGE_ALIGN(a) ((void*)((uintptr_t)(a) / PAGE_SIZE * PAGE_SIZE))

typedef u64 pml4e_t;
typedef u64 pdpte_t;
typedef u64 pde_t;
typedef u64 pte_t;

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

static inline void tlb_flush_single(void* addr)
{
    asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

static inline void tlb_flush_all(void)
{
    asm volatile("movq %%cr3, %%rax\n"
                 "movq %%rax, %%cr3"
                 :
                 :
                 : "rax", "memory");
}

/*
 * Map a page table
 * Notes: 
 * This function will automatically align vaddr, and it will NOT automatically align pt_paddr
 *
 * Return values:
 * -E2BIG: pt_paddr is too big
 * -EINVAL: Invalid flags, or vaddr is non-canonical
 * -EADDRNOTAVAIL: pml4/pdpt entries are not present
 * -EBUSY: Page directory entry already present
 * 0: Success
 */
int map_page_table(void* vaddr, const pte_t* pt_paddr, u64 flags);

/*
 * Unmap a page table
 * Notes:
 * vaddr is automatically aligned
 *
 * Return values:
 * -EINVAL: vaddr is non-canonical
 * -EADDRNOTAVAIL: pml4/pdpt entries are not present
 * 0: Success
 */
int unmap_page_table(void* vaddr);

/*
 * Map a page into memory
 * Notes:
 * This function automatically aligns vaddr and paddr, be aware of that to see if you need to map more pages
 *
 * Return values:
 * -E2BIG: paddr is bigger than the amount the CPU supports
 * -EINVAL: Invalid flags, or vaddr is non-canonical
 * -EADDRNOTAVAIL: pml4/pdpt/pd entries are not present
 * -EBUSY: Page table entry already present
 * 0: Success
 */
int map_page(void* vaddr, const void* paddr, u64 flags);

/*
 * Unmap a page
 * Notes:
 * This function automatically aligns vaddr, be aware of that to see if you need to unmap more pages
 *
 * Return values:
 * -EINVAL: vaddr is non-canonical
 * -EADDRNOTAVAIL: pml4/pdpt/pd entries are not present
 * 0: Success
 */
int unmap_page(void* vaddr);

/*
 * Map several pages
 * Notes:
 * This automatically aligns vaddr and paddr, make sure you don't need to map more pages
 * This will also unmap every page it successfully mapped if it fails
 *
 * Return values:
 * The exact same as map_page
 */
int map_pages(void* vaddr, const void* paddr, u64 flags, size_t count);

/*
 * Unmap several pages
 * Notes:
 * This automatically aligns vaddr and paddr, make sure you don't need to unmap more pages
 * This does NOT remap the pages that it previously unmapped on error if it doesn't fail on the first unmapping
 *
 * Return values:
 * The exact same as unmap_page
 */
int unmap_pages(void* vaddr, size_t count);

#endif /* __ASSEMBLY_FILE__ */
