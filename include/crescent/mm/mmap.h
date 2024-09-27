#pragma once

#include <crescent/types.h>
#include <crescent/core/locking.h>
#include <crescent/mm/gfp.h>

#define PAGE_ALIGN_4K(a) ((uintptr_t)(a) & ~(0x1000 - 1))
#define PAGE_ALIGN_2M(a) ((uintptr_t)(a) & ~(0x200000 - 1))

struct vm_ctx {
    unsigned long* ctx;
    spinlock_t lock;
};

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
 * @brief Check if a page is a 2MiB page or not
 *
 * @param ctx The virtual memory context to use
 * @param virtual The virtual address to check
 *
 * @retval true The page is a 2MiB page
 * @retval false The page is a 4K page
 */
bool vm_is_huge_page(struct vm_ctx* ctx, void* virtual);

/**
 * @brief Get the physical address of a virtual address
 *
 * @param ctx The virtual memory context to use
 * @param virtual The virtual address to get the physical address from
 *
 * @return (void*)-1 on failure, otherwise it returns the physical address
 */
void* vm_get_physaddr(struct vm_ctx* ctx, void* virtual);

/**
 * @brief Map a page
 *
 * This function should only be used if you have a specific physical address you want
 * to map to.
 *
 * @param ctx The virtual memory context to use
 * @param virtual The virtual address to map
 * @param physical The physical address to map the virtual address to
 * @param mmu_flags The MMU flags for the page
 *
 * @retval 0 Successful
 * @retval -EFAULT virtual is non-canonical, or physical is bigger than the CPU supports
 * @retval -EINVAL The mmu_flags are invalid
 * @retval -ENOMEM No memory to allocate another page table
 * @retval -EADDRINUSE The address is already mapped
 * @retval -ERANGE Tried mapping a 4K page in a 2MiB page area
 */
int vm_map_page(struct vm_ctx* ctx, void* virtual, void* physical, unsigned long mmu_flags);

/**
 * @brief Unmap a page
 *
 * @param ctx The virtual memory context to use
 * @param virtual The address to unmap
 *
 * @retval 0 Successful
 * @retval -EFAULT virtual is non-canonical
 * @retval -ENOENT Page table entry not present
 */
int vm_unmap_page(struct vm_ctx* ctx, void* virtual);

/**
 * @brief Allocate physical memory and return a virtual address to it
 *
 * The only time virtual should not be NULL is when you've already allocated
 * some virtual pages for it (via alloc_vpages function). If you don't do this,
 * you WILL cause problems.
 *
 * On error, this function does NOT free any virtual pages if virtual is not NULL,
 * and it will unmap any pages it previously successfully mapped.
 *
 * Values for errno:
 *  -ENOMEM No memory available
 *  -EINVAL Invalid MMU flags, or size is 0
 *  -EADDRINUSE Address is already mapped
 *  -ERANGE Tried to map a 4K page in a 2MiB page area
 *  -EFAULT virtual is non-canonical
 *
 *
 * @param virtual[in] The virtual address to map, NULL if you don't care
 * @param size[in] The size of the allocation
 * @param mmu_flags[in] The MMU flags to use
 * @param gfp_flags[in] The GFP flags for conditions of physical and virtual memory allocations
 * @param errno[out] The pointer to an errno variable if this function were to error
 *
 * @return (void*)-1 on error, otherwise you'll get a virtual address
 */
void* kmmap(void* virtual, size_t size, unsigned long mmu_flags, gfp_t gfp_flags, int* errno);

/**
 * @brief Unmap a virtual address and free the physical memory allocated
 *
 * If you've manually allocated the virtual address (via alloc_vpages), you should set the free_virtual
 * parameter to false so that you can manually free them yourself.
 *
 * If an error gets returned, this function will not attempt to continue unmapping any further,
 * and will not remap any pages that were unmapped.
 *
 * @param virtual The virtual address to map
 * @param size The size of the allocation
 * @param gfp_flags The GFP flags you used with kmmap
 * @param free_virtual Controls whether the function should free the virtual pages or not
 *
 * @retval -EFAULT virtual is -1, or another bad virtual address
 * @retval -EINVAL size is zero
 * @retval -EADDRNOTAVAIL Failed to get the physical address of a page (should not happen)
 * @retval -ENOENT Page table entry not present (this should not happen)
 */
int kmunmap(void* virtual, size_t size, gfp_t gfp_flags, bool free_virtual);

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
