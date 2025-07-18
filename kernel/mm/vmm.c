#include <crescent/common.h>
#include <crescent/compiler.h>
#include <crescent/asm/ctl.h>
#include <crescent/core/cpu.h>
#include <crescent/core/locking.h>
#include <crescent/core/limine.h>
#include <crescent/core/panic.h>
#include <crescent/core/trace.h>
#include <crescent/core/printk.h>
#include <crescent/mm/buddy.h>
#include <crescent/mm/vmm.h>
#include <crescent/mm/vma.h>
#include <crescent/lib/string.h>
#include "hhdm.h"
#include "pagetable.h"

#define GUARD_4KPAGE_COUNT 1

static unsigned long mmu_to_pt(mmuflags_t mmu_flags) {
	if (mmu_flags & MMU_CACHE_DISABLE && mmu_flags & MMU_WRITETHROUGH)
		return 0;

	unsigned long pt_flags = 0;
	if (mmu_flags & MMU_READ)
		pt_flags |= PT_PRESENT;
	if (mmu_flags & MMU_WRITE)
		pt_flags |= PT_READ_WRITE;
	if (mmu_flags & MMU_USER)
		pt_flags |= PT_USER_SUPERVISOR;

	if (mmu_flags & MMU_WRITETHROUGH)
		pt_flags |= PT_WRITETHROUGH;
	else if (mmu_flags & MMU_CACHE_DISABLE)
		pt_flags |= PT_CACHE_DISABLE;

	if (!(mmu_flags & MMU_EXEC))
		pt_flags |= PT_NX;

	return pt_flags;
}

static inline bool vmap_validate_flags(int flags) {
	if ((flags & VMM_IOMEM && flags & VMM_ALLOC) ||
			(flags & VMM_PHYSICAL && flags & VMM_ALLOC) ||
			(flags & VMM_NOREPLACE && !(flags & VMM_FIXED)))
		return false;

	return true;
}

static inline bool vprotect_validate_flags(int flags) {
	return flags == 0;
}

static inline bool vunmap_validate_flags(int flags) {
	return flags == 0;
}

static struct mm kernel_mm_struct;

static bool handle_pagetable_error(pte_t* pagetable, 
		void* virtual, physaddr_t physical, unsigned long pt_flags, 
		int err, int flags) {
	if (err == 0)
		return true;

	/* Simply update the mapping if we want a fixed mapping */
	if (err == -EEXIST && flags & VMM_FIXED) {
		assert(!(flags & VMM_NOREPLACE));
		err = pagetable_update(pagetable, virtual, physical, pt_flags);
		if (err)
			return false;
		return true;
	}

	return false;
}

void* vmap(void* hint, size_t size, mmuflags_t mmu_flags, int flags, void* optional) {
	if (!vmap_validate_flags(flags))
		return NULL;
	unsigned long pt_flags = mmu_to_pt(mmu_flags);
	if (pt_flags == 0)
		return NULL;

	const size_t page_size = flags & VMM_HUGEPAGE_2M ? HUGEPAGE_2M_SIZE : PAGE_SIZE;
	unsigned long page_count = (size + page_size - 1) / page_size;
	if (page_count == 0)
		return NULL;
	const unsigned int page_size_order = get_order(page_size);

	if (flags & VMM_IOMEM)
		flags |= VMM_PHYSICAL;
	if (flags & VMM_HUGEPAGE_2M)
		pt_flags |= PT_HUGEPAGE;

	u8* virtual;
	int err = vma_map(&kernel_mm_struct, hint, size, mmu_flags, flags, (void**)&virtual);
	if (err)
		return NULL;

	u8* ret = virtual;
	pte_t* pagetable = current_cpu()->mm_struct->pagetable;

	unsigned long irq;
	spinlock_lock_irq_save(&kernel_mm_struct.vma_list_lock, &irq);

	unsigned long mapped = 0;
	if (flags & VMM_PHYSICAL) {
		if (!optional)
			goto cleanup;
		physaddr_t physical = *(physaddr_t*)optional;
		if (physical % page_size) {
			err = -EINVAL;
			goto cleanup;
		}

		while (page_count--) {
			err = pagetable_map(pagetable, virtual, physical, pt_flags);
			if (!handle_pagetable_error(pagetable, virtual, physical, pt_flags, err, flags))
				goto cleanup;

			mapped++;
			virtual += page_size;
			physical += page_size;
		}
	} else if (flags & VMM_ALLOC) {
		mm_t mm_flags = MM_ZONE_NORMAL;
		if (optional)
			mm_flags = *(mm_t*)optional;

		while (page_count--) {
			physaddr_t physical = alloc_pages(mm_flags, page_size_order);
			if (!physical)
				goto cleanup;
			err = pagetable_map(pagetable, virtual, physical, pt_flags);
			if (!handle_pagetable_error(pagetable, virtual, physical, pt_flags, err, flags))
				goto cleanup;

			mapped++;
			virtual += page_size;
		}
	} else {
		goto cleanup;
	}

	tlb_flush_range(ret, size);
	spinlock_unlock_irq_restore(&kernel_mm_struct.vma_list_lock, &irq);
	return ret;
cleanup:
	while (mapped--) {
		virtual -= page_size;

		physaddr_t physical = 0;
		if (flags & VMM_ALLOC)
			physical = pagetable_get_physical(pagetable, virtual);

		err = pagetable_unmap(pagetable, virtual);
		tlb_flush_range(virtual, page_size);
		if (err)
			printk("mm: pagetable_unmap failed in cleanup of %s (err: %i)\n", __func__, err);

		/* 
		 * This isn't done in the flags & VMM_ALLOC since we want the reference 
		 * to the physical address to be gone in the page tables including in the TLB 
		 */
		if (physical && err == 0)
			free_pages(physical, page_size_order);
	}

	err = vma_unmap(&kernel_mm_struct, virtual, size);
	if (err)
		printk(PRINTK_ERR "mm: vma_unmap failed in error handling of %s (err: %i)\n", __func__, err);
	spinlock_unlock_irq_restore(&kernel_mm_struct.vma_list_lock, &irq);
	return NULL;
}

int vprotect(void* virtual, size_t size, mmuflags_t mmu_flags, int flags) {
	if ((uintptr_t)virtual % PAGE_SIZE || size == 0 || !vprotect_validate_flags(flags))
		return -EINVAL;
	unsigned long pt_flags = mmu_to_pt(mmu_flags);
	if (pt_flags == 0)
		return -EINVAL;

	pte_t* pagetable = current_cpu()->mm_struct->pagetable;
	int err = 0;

	unsigned long irq;
	spinlock_lock_irq_save(&kernel_mm_struct.vma_list_lock, &irq);

	void* const end = (u8*)virtual + size;
	while (virtual < end) {
		struct vma* vma = vma_find(&kernel_mm_struct, virtual);
		if (!vma) {
			err = -ENOENT;
			goto out;
		}

		size_t page_size = PAGE_SIZE;
		if (vma->flags & VMM_HUGEPAGE_2M) {
			page_size = HUGEPAGE_2M_SIZE;
			pt_flags |= PT_HUGEPAGE;
		}

		err = vma_protect(&kernel_mm_struct, virtual, page_size, mmu_flags);
		if (err)
			goto out;
		err = pagetable_update(pagetable, virtual, pagetable_get_physical(pagetable, virtual), pt_flags);
		if (err)
			goto out;
		tlb_flush_range(virtual, page_size);

		pt_flags &= ~PT_HUGEPAGE;
		virtual = (u8*)virtual + page_size;
	}
out:
	spinlock_unlock_irq_restore(&kernel_mm_struct.vma_list_lock, &irq);
	return err;
}

int vunmap(void* virtual, size_t size, int flags) {
	if ((uintptr_t)virtual % PAGE_SIZE || size == 0 || !vunmap_validate_flags(flags))
		return -EINVAL;

	pte_t* pagetable = current_cpu()->mm_struct->pagetable;
	int err;

	unsigned long irq;
	spinlock_lock_irq_save(&kernel_mm_struct.vma_list_lock, &irq);

	void* const end = (u8*)virtual + size;
	while (virtual < end) {
		struct vma* vma = vma_find(&kernel_mm_struct, virtual);
		if (!vma) {
			err = -ENOENT;
			goto err;
		}

		const size_t page_size = vma->flags & VMM_HUGEPAGE_2M ? HUGEPAGE_2M_SIZE : PAGE_SIZE;
		physaddr_t physical = 0;
		if (vma->flags & VMM_ALLOC)
			physical = pagetable_get_physical(pagetable, virtual);

		vma_unmap(&kernel_mm_struct, virtual, page_size);
		err = pagetable_unmap(pagetable, virtual);
		if (err) {
			printk(PRINTK_CRIT "mm: Failed to unmap kernel page, err: %i", err);
			goto err;
		}
		tlb_flush_range(virtual, page_size);

		if (physical && err == 0)
			free_pages(physical, get_order(page_size));

		virtual = (u8*)virtual + page_size;
	}
err:
	spinlock_unlock_irq_restore(&kernel_mm_struct.vma_list_lock, &irq);
	return err;
}

void __iomem* iomap(physaddr_t physical, size_t size, mmuflags_t mmu_flags) {
	if (!(mmu_flags & MMU_WRITETHROUGH))
		mmu_flags |= MMU_CACHE_DISABLE;

	size_t page_offset = physical % PAGE_SIZE;
	physaddr_t _physical = physical - page_offset;
	u8 __iomem* ret = vmap(NULL, size + page_offset, mmu_flags, VMM_IOMEM, &_physical);
	if (!ret)
		return NULL;

	return ret + page_offset;
}

int iounmap(void __iomem* virtual, size_t size) {
	size_t page_offset = (uintptr_t)virtual % PAGE_SIZE;
	void* _virtual = (u8*)virtual - page_offset;
	return vunmap(_virtual, size + page_offset, 0);
}

void vmm_init(void) {
	pagetable_init();

	/* No need to check the pointer, the system would triple fault if an invalid page table was in cr3 */
	struct cpu* cpu = current_cpu();
	cpu->mm_struct = &kernel_mm_struct;
	pte_t* cr3 = hhdm_virtual(ctl3_read());
	cpu->mm_struct->pagetable = cr3;

	int best = 0;
	int best_len = 0;
	int current = 0;
	int current_len = 0;

	/* Check for the biggest contiguous space, HHDM mappings can be anywhere in the higher half */
	for (int i = 256; i < PTE_COUNT - 1; i++) {
		if (!(cr3[i] & PT_PRESENT)) {
			if (current_len == 0)
				current = i;
			current_len++;
		} else {
			if (current_len > best_len) {
				best_len = current_len;
				best = current;
			}
			current_len = 0;
		}
	}

	if (current_len > best_len) {
		best_len = current_len;
		best = current;
	}

	kernel_mm_struct.mmap_start = pagetable_get_base_address_from_top_index(best);
	kernel_mm_struct.mmap_end = pagetable_get_base_address_from_top_index(best + best_len);
}

void vmm_switch_mm_struct(struct mm* mm) {
	unsigned long irq = local_irq_save();

	atomic_thread_fence(ATOMIC_SEQ_CST);
	ctl3_write(hhdm_physical(mm->pagetable));
	current_cpu()->mm_struct = mm;

	local_irq_restore(irq);
}
