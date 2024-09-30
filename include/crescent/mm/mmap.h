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

enum pt_flags {
    PT_PRESENT = (1 << 0),
    PT_READ_WRITE = (1 << 1),
    PT_USER_SUPERVISOR = (1 << 2),
    PT_WRITE_THROUGH = (1 << 3),
    PT_CACHE_DISABLE = (1 << 4),
    PT_ACCESSED = (1 << 5),
    PT_DIRTY = (1 << 6),
    PT_LARGE = (1 << 7),
    PT_GLOBAL = (1 << 8),
    PT_NX = (1ul << 63)
};

enum mmu_flags {
    MMU_READ = (1 << 0),
    MMU_WRITE = (1 << 1),
    MMU_USER = (1 << 2),
    MMU_WRITE_THROUGH = (1 << 3),
    MMU_CACHE_DISABLE = (1 << 4),
    MMU_EXEC = (1ul << 63)
};

enum kmmap_flags {
    KMMAP_ALLOC = (1 << 0),
    KMMAP_PHYS = (1 << 1)
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
 * @return NULL on failure, otherwise it returns the physical address
 */
void* vm_get_physaddr(struct vm_ctx* ctx, void* virtual);

/**
 * @brief Map some memory to the virtual address space
 *
 * At the moment, the virtual parameter will be always treated as if it were NULL.
 * This will change
 *
 * @param[in] virtual The hint of the virtual address to map
 * @param[in] size The size of the mapping
 * @param[in] flags kmmap flags
 * @param[in] mmu_flags MMU flags for the pages
 * @param[in] gfp_flags Flags for how the physical/virtual pages should be allocated
 * @param[in] private Physical address to map to, ignored if KMMAP_PHYS is not in flags
 * @param[out] errno The error code gets stored here if an error happens
 *
 * Values for *errno:
 *  0: Success
 *  -EINVAL: size == 0 or invalid mmu flags or invalid mmu flag combination
 *  -ENOMEM: No memory available
 *  -EADDRINUSE: This is a bug
 *  -ERANGE: This is also a bug
 *
 * @return NULL if no virtual address is available, otherwise you get a mapped virtual address
 */
void* kmmap(void* virtual, size_t size, unsigned int flags, 
        unsigned long mmu_flags, gfp_t gfp_flags, void* private, int* errno);

/**
 * @brief Change the MMU flags of a page
 *
 * @param virtual The virtual address to change the MMU flags for
 * @param size The size of the region
 * @param mmu_flags The new MMU flags to use
 *
 * @retval 0 Success
 */
int kmprotect(void* virtual, size_t size, unsigned long mmu_flags);

/**
 * @brief Unmap memory from the virtual address space
 *
 * @param virtual The virtual address to unmap
 * @param size The size used in kmmap
 * @param flags The kmmap flags used in kmmap
 *
 * @retval 0 Success
 * @retval -EFAULT Bad virtual address
 * @retval -EINVAL size is 0
 * @retval -EADDRNOTAVAIL Physical address not available, likely a bug
 */
int kmunmap(void* virtual, size_t size, unsigned int flags);

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
