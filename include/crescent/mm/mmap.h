#pragma once

#include <crescent/core/locking.h>

#define PAGE_ALIGN_4K(a) ((uintptr_t)(a) & ~(0x1000 - 1))
#define PAGE_ALIGN_2M(a) ((uintptr_t)(a) & ~(0x200000 - 1))

enum mmu_flags {
    MMU_FLAG_PRESENT = (1 << 0),
    MMU_FLAG_READ_WRITE = (1 << 1),
    MMU_FLAG_USER_SUPERVISOR = (1 << 2),
    MMU_FLAG_WRITE_THROUGH = (1 << 3),
    MMU_FLAG_CACHE_DISABLE = (1 << 4),
    MMU_FLAG_ACCESSED = (1 << 5),
    MMU_FLAG_LARGE = (1 << 7),
    MMU_FLAG_NX = (1ul << 63)
};

void mmap_init(void);

/**
 * @brief Get the physical address of a page
 * @param virtaddr The virtual address
 * @return NULL if unmapped, otherwise it returns the physical address
 */
void* get_physaddr(const void* virtaddr);

/**
 * @brief Map a page
 *
 * virtaddr and physaddr will be automatically aligned to 4K or 2M depending on if
 * MMU_FLAG_LARGE is in the flags
 *
 * @param virtaddr The virtual address to map
 * @param physaddr The physical address to map virtaddr to
 * @param mmu_flags The MMU permissions for the page
 *
 * @retval 0 Success
 * @retval -EFAULT virtaddr is non-canonical or physaddr is too big
 * @retval -EINVAL flags are invalid
 * @retval -ENOMEM Cannot allocate memory for page tables
 * @retval -EADDRINUSE Address is already mapped
 */
int map_page(void* virtaddr, void* physaddr, unsigned long mmu_flags);

/**
 * @brief Unmap a page
 *
 * virtaddr is automatically aligned to 4K or 2M depending on the MMU flags
 * for the page
 *
 * @param virtaddr The virtual address to unmap
 *
 * @retval 0 Success
 * @retval -EFAULT virtaddr is non-canonical
 * @retval -ENOENT Page table entry not present
 */
int unmap_page(void* virtaddr);

/**
 * @brief Flush the TLB for a single page
 *
 * This only affects the processor that the instruction was executed on.
 * If you want to flush the TLB for all processors, you need a TLB shootdown (not implemented).
 *
 * @param addr The address to flush
 */
static inline void _tlb_flush_single(void* addr) {
    __asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}
