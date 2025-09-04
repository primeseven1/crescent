#pragma once

#include <crescent/mm/vmm.h>
#include <crescent/mm/mm.h>

#define PTE_COUNT 512

enum pt_flags {
	PT_PRESENT = (1 << 0),
	PT_READ_WRITE = (1 << 1),
	PT_USER_SUPERVISOR = (1 << 2),
	PT_WRITETHROUGH = (1 << 3),
	PT_CACHE_DISABLE = (1 << 4),
	PT_ACCESSED = (1 << 5),
	PT_DIRTY = (1 << 6),
	PT_4K_PAT = (1 << 7),
	PT_HUGEPAGE = (1 << 7),
	PT_GLOBAL = (1 << 8),
	PT_HUGEPAGE_PAT = (1 << 12),
	PT_NX = (1ul << 63)
};

static inline void tlb_flush_single(void* virtual) {
	__asm__ volatile("invlpg (%0)" : : "r"(virtual) : "memory");
}

static inline void tlb_flush_range(void* virtual, size_t size) {
	unsigned long count = (size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	for (unsigned long i = 0; i < count; i++)
		tlb_flush_single((u8*)virtual + (PAGE_SIZE * i));
}

unsigned long pagetable_mmu_to_pt(mmuflags_t mmu_flags);

void tlb_invalidate(void* address, size_t size);

/**
 * @brief Map an entry into a page table
 *
 * -EINVAL is returned if virtual or physical are not page aligned.
 * Same goes with invalid pt_flags. This value is also returned if virtual is non-canonical,
 * or if physical is NULL.
 *
 * -EEXIST is returned if the PTE is already present.
 *
 * -ENOMEM is returned if no page tables could be allocated for the new mapping
 *
 * @param pagetable The page table to use
 * @param virtual The virtual address to map, must be page aligned
 * @param physical The physical address to map the virtual address to, must be page aligned, cannot be 0
 * @param pt_flags The page table flags to use
 *
 * @return -errno on failure
 */
int pagetable_map(pte_t* pagetable, void* virtual, physaddr_t physical, unsigned long pt_flags);

/**
 * @brief Update an entry in a page table
 *
 * -EINVAL is returned if virtual or physical are not page aligned.
 * Same goes with invalid pt_flags. This value is also returned if virtual is non-canonical,
 * or if physical is NULL.
 * 
 * -EFAULT is returned if the page sizes don't match (eg. trying to remap a 2MiB page to a 4K one)
 *
 * -ENOENT is returned if the PTE isn't present.
 *
 * @param pagetable The page table to use
 * @param virtual The virtual address to map, must be page aligned
 * @param physical The physical address to map, must be page aligned, cannot be 0
 * @param pt_flags The PT flags to use
 *
 * @return -errno on failure
 */
int pagetable_update(pte_t* pagetable, void* virtual, physaddr_t physical, unsigned long pt_flags);

/**
 * @brief Unmap an entry in a page table
 *
 * -EINVAL is returned if virtual isn't page aligned. This value is also returned if virtual is non-canonical.
 * -ENOENT is returned if the PTE isn't present
 *
 * @param pagetable The page table to use
 * @param virtual The virtual address to unmap, must be page aligned
 */
int pagetable_unmap(pte_t* pagetable, void* virtual);

/**
 * @brief Get the physical address of a mapping in a page table
 *
 * Returns NULL if the physical address isn't mapped.
 *
 * @param pagetable The page table to use
 * @param virtual The virtual address, does not need to be page aligned
 *
 * @return The physical address of the mapping
 */
physaddr_t pagetable_get_physical(pte_t* pagetable, const void* virtual);

/**
 * @brief Get the base address of a top level page table index
 * @param index The index
 * @return The base address
 */
void* pagetable_get_base_address_from_top_index(unsigned int index);

void pagetable_init(void);

struct prevpage {
	void* start;
	physaddr_t physical;
	size_t page_size;
	mmuflags_t mmu_flags;
	int vmm_flags;
	struct prevpage* next;
};

/**
 * @brief Save any info about any pages typically before overwriting them
 *
 * This function will not fail to save pages, as the MM_NOFAIL flag is used to allocate them.
 * Pointers returned are in HHDM.
 *
 * @param mm_struct The mm struct to use
 * @param virtual The virtual address of the pages
 * @param size The size of the region
 *
 * @return NULL if there are no pages to save, or you get a singly linked list 
 */
struct prevpage* prevpage_save(struct mm* mm_struct, u8* virtual, size_t size);

/**
 * @brief Restore the previous state on a failure
 *
 * @param mm_struct The mm struct to use
 * @param head The previous pages
 */
void prevpage_fail(struct mm* mm_struct, struct prevpage* head);

enum prevpage_flags {
	PREVPAGE_FREE_PREVIOUS = (1 << 0)
};

/**
 * @brief Cleaup after saving pages
 *
 * When the PREVPAGE_FREE_PREVIOUS flag is used, it frees the physical pages that were previously saved
 *
 * @param head The previous pages
 * @param flags Flags for what should be cleaned up
 */
void prevpage_success(struct prevpage* head, int flags);
