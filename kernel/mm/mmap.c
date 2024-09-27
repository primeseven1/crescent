#include <crescent/common.h>
#include <crescent/asm/cpuid.h>
#include <crescent/asm/ctl.h>
#include <crescent/core/printk.h>
#include <crescent/core/cpu.h>
#include <crescent/core/panic.h>
#include <crescent/mm/mmap.h>
#include <crescent/mm/hhdm.h>
#include <crescent/mm/vm_zone.h>
#include <crescent/mm/zone.h>
#include <crescent/lib/string.h>

static inline unsigned long* get_table_virtaddr(unsigned long entry) {
    entry &= ~(0xFFF | MMU_FLAG_NX);
    return hhdm_virtual((unsigned long*)entry);
}

static inline unsigned long* alloc_table(void) {
    unsigned long* page = alloc_page(GFP_PM_ZONE_NORMAL);
    if (!page)
        return NULL;
    memset(hhdm_virtual(page), 0, 4096);
    return page;
}

static inline void free_table(unsigned long entry) {
    entry &= ~(0xFFF | MMU_FLAG_NX);
    free_page((unsigned long*)entry);
}

static inline bool is_flags_valid(unsigned long flags) {
    flags &= ~(0xFFF | MMU_FLAG_NX);
    return !flags;
}

static inline bool is_virtaddr_canonical(const void* addr) {
    return ((uintptr_t)addr >> 47 == 0 || (uintptr_t)addr >> 47 == 0x1FFFF);
}

static inline bool is_physaddr_canonical(const void* addr) {
    static void* max_addr = NULL;
    if (unlikely(!max_addr)) {
        u32 eax, unused;
        cpuid(CPUID_EXT_LEAF_ADDRESS_SIZES, 0, &eax, &unused, &unused, &unused);
        max_addr = (void*)((1ull << (eax & 0xFF)));
    }
    return addr < max_addr;
}

/* 
 * Will contain every CPU's virtual address space context. 
 * The code that's gonna use this is not implemented 
 */
__attribute__((unused))
static struct vm_ctx** all_cpu_vm_ctx = NULL;
/* Lock for the kernel address space, since all pages for the kernel address space are shared */
static spinlock_t kas_lock = SPINLOCK_INITIALIZER;

static bool is_huge_page(struct vm_ctx* ctx, void* virtaddr) {
    unsigned long* pml4, *pdpt, *pd;
    u16 pml4_i, pdpt_i, pd_i;

    pml4 = ctx->ctx;
    pml4_i = ((uintptr_t)virtaddr >> 39) & 0x01FF;
    if (!(pml4[pml4_i] & MMU_FLAG_PRESENT))
        return false;

    pdpt = get_table_virtaddr(pml4[pml4_i]);
    pdpt_i = ((uintptr_t)virtaddr >> 30) & 0x01FF;
    if (!(pdpt[pdpt_i] & MMU_FLAG_PRESENT))
        return false;

    pd = get_table_virtaddr(pdpt[pdpt_i]);
    pd_i = ((uintptr_t)virtaddr >> 21) & 0x01FF;
    if (!(pd[pd_i] & MMU_FLAG_PRESENT))
        return false;
    if (pd[pd_i] & MMU_FLAG_LARGE)
        return true;

    return false;
}

static void* get_physaddr(struct vm_ctx* ctx, const void* virtaddr) {
    unsigned long* pml4, *pdpt, *pd, *pt;
    u16 pml4_i, pdpt_i, pd_i, pt_i;

    pml4 = ctx->ctx;
    pml4_i = ((uintptr_t)virtaddr >> 39) & 0x01FF;
    if (!(pml4[pml4_i] & MMU_FLAG_PRESENT))
        return (void*)-1;

    pdpt = get_table_virtaddr(pml4[pml4_i]);
    pdpt_i = ((uintptr_t)virtaddr >> 30) & 0x01FF;
    if (!(pdpt[pdpt_i] & MMU_FLAG_PRESENT))
        return (void*)-1;

    pd = get_table_virtaddr(pdpt[pdpt_i]);
    pd_i = ((uintptr_t)virtaddr >> 21) & 0x01FF;
    if (!(pd[pd_i] & MMU_FLAG_PRESENT))
        return (void*)-1;
    if (pd[pd_i] & MMU_FLAG_LARGE) {
        unsigned long offset = (uintptr_t)virtaddr - PAGE_ALIGN_2M(virtaddr);
        return (void*)((pd[pd_i] & ~(0xFFF | MMU_FLAG_NX)) + offset);
    }

    pt = get_table_virtaddr(pd[pd_i]);
    pt_i = ((uintptr_t)virtaddr >> 12) & 0x01FF;
    if (!(pt[pt_i] & MMU_FLAG_PRESENT))
        return (void*)-1;

    unsigned long offset = (uintptr_t)virtaddr - PAGE_ALIGN_4K(virtaddr);
    return (void*)((pt[pt_i] & ~(0xFFF | MMU_FLAG_NX)) + offset);
}

static int map_page(struct vm_ctx* ctx, void* virtaddr, void* physaddr, unsigned long mmu_flags) {
    unsigned long* pml4, *pdpt, *pd, *pt; 
    u16 pml4_i, pdpt_i, pd_i, pt_i;

    /* Used for keeping track of what page tables that needed to be allocated */
    bool alloc_pdpt = false;
    bool alloc_pd = false;

    int err;
 
    pml4 = ctx->ctx;
    pml4_i = ((uintptr_t)virtaddr >> 39) & 0x01FF;

    if (!(pml4[pml4_i] & MMU_FLAG_PRESENT)) {
        unsigned long* table = alloc_table();
        /* There are no resources to release, so just return */
        if (!table)
            return -ENOMEM;

        alloc_pdpt = true;
        pml4[pml4_i] = (uintptr_t)table | MMU_FLAG_PRESENT | MMU_FLAG_READ_WRITE;
    }

    pdpt = get_table_virtaddr(pml4[pml4_i]);
    pdpt_i = ((uintptr_t)virtaddr >> 30) & 0x01FF;

    if (!(pdpt[pdpt_i] & MMU_FLAG_PRESENT)) {
        unsigned long* table = alloc_table();
        if (!table) {
            err = -ENOMEM;
            goto err;
        }
        alloc_pd = true;
        pdpt[pdpt_i] = (uintptr_t)table | MMU_FLAG_PRESENT | MMU_FLAG_READ_WRITE;
    }

    pd = get_table_virtaddr(pdpt[pdpt_i]);
    pd_i = ((uintptr_t)virtaddr >> 21) & 0x01FF;

    if (mmu_flags & MMU_FLAG_LARGE) {
        if (pd[pd_i] & MMU_FLAG_PRESENT) {
            err = -EADDRINUSE;
            goto err;
        }

        /* Not aligning the address will cause problems with the mmu flags */
        physaddr = (void*)PAGE_ALIGN_2M(physaddr);
        pd[pd_i] = (uintptr_t)physaddr | mmu_flags;
        _tlb_flush_single(virtaddr);
        return 0;
    }

    /* 
     * Make sure that the PD does not have the huge page flag, 
     * since the PD maps directly to the physical address when set 
     */
    if (pd[pd_i] & MMU_FLAG_LARGE) {
        err = -ERANGE;
        goto err;
    } else if (!(pd[pd_i] & MMU_FLAG_PRESENT)) {
        unsigned long* table = alloc_table();
        if (!table) {
            err = -ENOMEM;
            goto err;
        }
        pd[pd_i] = (uintptr_t)table | MMU_FLAG_PRESENT | MMU_FLAG_READ_WRITE;
    }

    /* Now try mapping the 4K page */
    pt = get_table_virtaddr(pd[pd_i]);
    pt_i = ((uintptr_t)virtaddr >> 12) & 0x01FF;

    /* This can only happen if no page tables were allocated, so returning is fine */
    if (pt[pt_i] & MMU_FLAG_PRESENT)
        return -EADDRINUSE;

    /* Like with 2MiB pages, not aligning will cause problems with the mmu flags */
    physaddr = (void*)PAGE_ALIGN_4K(physaddr); 
    pt[pt_i] = (uintptr_t)physaddr | mmu_flags;
    _tlb_flush_single(virtaddr);
    return 0;
err:
    if (alloc_pd) {
        free_table(pdpt[pdpt_i]);
        pdpt[pdpt_i] = 0;
    }
    if (alloc_pdpt) {
        free_table(pml4[pml4_i]);
        pml4[pml4_i] = 0;
    }

    return err;
}

/* Attempt to free page tables once a page is unmapped */
static void try_free_tables(unsigned long* pml4, unsigned long* pdpt, 
        u16 pml4_i, unsigned long* pd, u16 pdpt_i, unsigned long* pt, u16 pd_i) {
    /* pt is null if a huge page is unmapped */
    if (pt) {
        for (u16 i = 0; i < 512; i++) {
            if (pt[i] & MMU_FLAG_PRESENT)
                return;
        }

        free_table((uintptr_t)hhdm_physical(pt));
        pd[pd_i] = 0;
    }

    for (u16 i = 0; i < 512; i++) {
        if (pd[i] & MMU_FLAG_PRESENT)
            return;
    }

    free_table((uintptr_t)hhdm_physical(pd));
    pdpt[pdpt_i] = 0;

    for (u16 i = 0; i < 512; i++) {
        if (pdpt[i] & MMU_FLAG_PRESENT)
            return;
    }

    free_table((uintptr_t)hhdm_physical(pdpt));
    pml4[pml4_i] = 0;
}

static int unmap_page(struct vm_ctx* ctx, void* virtaddr) {
    unsigned long* pml4, *pdpt, *pd, *pt;
    u16 pml4_i, pdpt_i, pd_i, pt_i;

    pml4 = ctx->ctx;
    pml4_i = ((uintptr_t)virtaddr) >> 39 & 0x01FF;
    if (!(pml4[pml4_i] & MMU_FLAG_PRESENT))
        return -ENOENT;

    pdpt = get_table_virtaddr(pml4[pml4_i]);
    pdpt_i = ((uintptr_t)virtaddr >> 30) & 0x01FF;
    if (!(pdpt[pdpt_i] & MMU_FLAG_PRESENT))
        return -ENOENT;

    /* Handle 2MiB pages */
    pd = get_table_virtaddr(pdpt[pdpt_i]);
    pd_i = ((uintptr_t)virtaddr >> 21) & 0x01FF;
    if (pd[pd_i] & MMU_FLAG_LARGE) {
        pd[pd_i] = 0;
        _tlb_flush_single(virtaddr);
        try_free_tables(pml4, pdpt, pml4_i, pd, pdpt_i, NULL, 0);
        return 0;
    }

    /* Page is a 4K page, continue checking */
    if (!(pd[pd_i] & MMU_FLAG_PRESENT))
        return -ENOENT;

    pt = get_table_virtaddr(pd[pd_i]);
    pt_i = ((uintptr_t)virtaddr >> 12) & 0x01FF;

    pt[pt_i] = 0;
    _tlb_flush_single(virtaddr);
    try_free_tables(pml4, pdpt, pml4_i, pd, pdpt_i, pt, pd_i);
    return 0;
}

bool vm_is_huge_page(struct vm_ctx* ctx, void* virtual) {
    if (!is_virtaddr_canonical(virtual))
        return false;

    bool ret;
    unsigned long lock_flags;

    u16 top_lvl_entry = (uintptr_t)virtual >> 39 & 0x01FF;
    if (top_lvl_entry >= 256) {
        spinlock_lock_irq_save(&kas_lock, &lock_flags);
        ret = is_huge_page(ctx, virtual);
        spinlock_unlock_irq_restore(&kas_lock, &lock_flags);
    } else {
        spinlock_lock_irq_save(&ctx->lock, &lock_flags);
        ret = is_huge_page(ctx, virtual);
        spinlock_unlock_irq_restore(&ctx->lock, &lock_flags);
    }

    return ret;
}

void* vm_get_physaddr(struct vm_ctx* ctx, void* virtual) {
    if (!is_virtaddr_canonical(virtual))
        return (void*)-1;

    void* ret;
    unsigned long lock_flags;

    u16 top_lvl_entry = (uintptr_t)virtual >> 39 & 0x01FF;
    if (top_lvl_entry >= 256) {
        spinlock_lock_irq_save(&kas_lock, &lock_flags);
        ret = get_physaddr(ctx, virtual);
        spinlock_unlock_irq_restore(&kas_lock, &lock_flags);
    } else {
        spinlock_lock_irq_save(&ctx->lock, &lock_flags);
        ret = get_physaddr(ctx, virtual);
        spinlock_unlock_irq_restore(&ctx->lock, &lock_flags);
    }

    return ret;
}

int vm_map_page(struct vm_ctx* ctx, void* virtual, void* physical, unsigned long mmu_flags) {
    if (!is_virtaddr_canonical(virtual) || !is_physaddr_canonical(physical))
        return -EFAULT;
    if (!is_flags_valid(mmu_flags))
        return -EINVAL;

    int err;
    unsigned long lock_flags;

    /* TODO: Add TLB shootdown when the time comes */
    u16 top_lvl_entry = (uintptr_t)virtual >> 39 & 0x01FF;
    if (top_lvl_entry >= 256) {
        spinlock_lock_irq_save(&kas_lock, &lock_flags);
        err = map_page(ctx, virtual, physical, mmu_flags);
        spinlock_unlock_irq_restore(&kas_lock, &lock_flags);
    } else {
        spinlock_lock_irq_save(&ctx->lock, &lock_flags);
        err = map_page(ctx, virtual, physical, mmu_flags);
        spinlock_unlock_irq_restore(&ctx->lock, &lock_flags);
    }

    return err;
}

static int vm_map_pages(struct vm_ctx* ctx, 
        void* virtual, void* physical, unsigned long mmu_flags, unsigned long count) {
    if (!is_virtaddr_canonical(virtual) || !is_physaddr_canonical(physical))
        return -EFAULT;
    if (!is_flags_valid(mmu_flags))
        return -EINVAL;

    int err = 0;
    unsigned long lock_flags;
    size_t page_size = mmu_flags & MMU_FLAG_LARGE ? 0x200000 : 0x1000;

    u16 top_lvl_entry = (uintptr_t)virtual >> 39 & 0x01FF;
    if (top_lvl_entry >= 256)
        spinlock_lock_irq_save(&kas_lock, &lock_flags);
    else
        spinlock_lock_irq_save(&ctx->lock, &lock_flags);

    unsigned long pages_mapped = 0;
    while (count--) {
        err = map_page(ctx, virtual, physical, mmu_flags);
        if (err) {
            while (pages_mapped--) {
                virtual = (u8*)virtual - page_size; 
                unmap_page(ctx, virtual);
            }

            break;
        }
        
        virtual = (u8*)virtual + page_size;
        physical = (u8*)physical + page_size;
    }

    if (top_lvl_entry >= 256)
        spinlock_unlock_irq_restore(&kas_lock, &lock_flags);
    else
        spinlock_unlock_irq_restore(&ctx->lock, &lock_flags);
    return err;
}

int vm_unmap_page(struct vm_ctx* ctx, void* virtual) {
    if (!is_virtaddr_canonical(virtual))
        return -EFAULT;

    int err;
    unsigned long lock_flags;

    /* TODO: Add TLB shootdown whenever the time comes */
    u16 top_lvl_entry = (uintptr_t)virtual >> 39 & 0x01FF;
    if (top_lvl_entry >= 256) {
        spinlock_lock_irq_save(&kas_lock, &lock_flags);
        err = unmap_page(ctx, virtual);
        spinlock_unlock_irq_restore(&kas_lock, &lock_flags);
    } else {
        spinlock_lock_irq_save(&ctx->lock, &lock_flags);
        err = unmap_page(ctx, virtual);
        spinlock_unlock_irq_restore(&ctx->lock, &lock_flags);
    }

    return err;
}

static int vm_unmap_pages(struct vm_ctx* ctx, void* virtual, unsigned long count) {
     if (!is_virtaddr_canonical(virtual))
        return -EFAULT;

    int err = 0;
    unsigned long lock_flags;

    u16 top_lvl_entry = (uintptr_t)virtual >> 39 & 0x01FF;
    if (top_lvl_entry >= 256)
        spinlock_lock_irq_save(&kas_lock, &lock_flags);
    else
        spinlock_lock_irq_save(&ctx->lock, &lock_flags);

    size_t page_size = is_huge_page(ctx, virtual);
    while (count--) {
        err = unmap_page(ctx, virtual);
        if (err)
            break;

        virtual = (u8*)virtual + page_size;
    }

    if (top_lvl_entry >= 256)
        spinlock_unlock_irq_restore(&kas_lock, &lock_flags);
    else
        spinlock_unlock_irq_restore(&ctx->lock, &lock_flags);
    return err;
}

void* kmmap(void* virtual, size_t size, unsigned long mmu_flags, unsigned int gfp_flags, int* errno) {
    if (size == 0) {
        *errno = -EINVAL;
        return (void*)-1;
    }

    mmu_flags |= MMU_FLAG_PRESENT;

    size_t page_size;
    if (gfp_flags & GFP_VM_LARGE_PAGE) {
        mmu_flags |= MMU_FLAG_LARGE;
        virtual = (void*)PAGE_ALIGN_2M(virtual);
        page_size = 0x200000;
    } else {
        mmu_flags &= ~MMU_FLAG_LARGE;
        virtual = (void*)PAGE_ALIGN_4K(virtual);
        page_size = 0x1000;
    }

    /* If virtual is NULL, error handling needs to be done differently */
    bool virtual_null = (bool)virtual;

    unsigned long page_count = size & (page_size - 1) ? size / page_size + 1 : size / page_size;
    unsigned int order = get_order(size);

    /* First try to allocate the virtual pages */
    unsigned int gfp_virt = gfp_flags & GFP_VM_FLAGS_MASK;
    if (!virtual) {
        virtual = alloc_vpages(gfp_virt, order);
        if (!virtual) {
            *errno = -ENOMEM;
            return (void*)-1;
        }
    }

    unsigned int gfp_phys = gfp_flags & GFP_PM_FLAGS_MASK;

    /* If the block must be contiguous, allocate contiguously */
    if (gfp_flags & GFP_PM_CONTIGUOUS) {
        void* physical = alloc_pages(gfp_phys, order);
        if (!physical) {
            if (virtual_null)
                free_vpages(virtual, order);
            *errno = -ENOMEM;
            return (void*)-1;
        }
        *errno = vm_map_pages(&_cpu()->vm_ctx, virtual, physical, mmu_flags, page_count);
        if (*errno) {
            free_pages(physical, order);
            if (virtual_null)
                free_vpages(virtual, order);
            return (void*)-1;
        }
        return virtual;
    }

    struct vm_ctx* vm_ctx = &_cpu()->vm_ctx;
    void* saved_virtual = virtual;

    /* Try to allocate the physical memory non-contiguously instead */
    unsigned long pages_mapped = 0;
    unsigned int page_size_order = get_order(page_size); 
    while (page_count) {
        void* physical = alloc_pages(gfp_phys, page_size_order);
        if (!physical) {
            *errno = -ENOMEM;
            break;
        }
        *errno = vm_map_page(vm_ctx, virtual, physical, mmu_flags);
        if (*errno)
            break;
        virtual = (u8*)virtual + page_size;
        page_count--;
        pages_mapped++;
    }

    /* All pages were successfully mapped, so return here */
    if (page_count == 0)
        return saved_virtual;
    
    /* Not all pages could be mapped, so get the physical address and free it, then unmap it */
    while (pages_mapped--) {
        virtual = (u8*)virtual - page_size;
        void* physical = vm_get_physaddr(vm_ctx, virtual);
        free_pages(physical, page_size_order);
        vm_unmap_page(vm_ctx, virtual);
    }
   
    if (virtual_null)
        free_vpages(saved_virtual, order);

    return (void*)-1;
}

int kmunmap(void* virtual, size_t size, unsigned int gfp_flags, bool free_virtual) {
    if (virtual == (void*)-1)
        return -EFAULT;
    if (size == 0)
        return -EINVAL;

    struct vm_ctx* vm_ctx = &_cpu()->vm_ctx;
    unsigned int order = get_order(size);

    size_t page_size = gfp_flags & GFP_VM_LARGE_PAGE ? 0x200000 : 0x1000;
    unsigned int page_size_order = get_order(page_size);

    void* saved_virtual = virtual;

    size_t page_count = size & (page_size - 1) ? size / page_size + 1 : size / page_size;
    if (gfp_flags & GFP_PM_CONTIGUOUS) {
        void* physical = vm_get_physaddr(vm_ctx, virtual);
        if (unlikely(physical == (void*)-1))
            return -EADDRNOTAVAIL;

        vm_unmap_pages(vm_ctx, virtual, page_count);
        free_pages(physical, order);
    } else {
        while (page_count--) {
            void* physical = vm_get_physaddr(vm_ctx, virtual);
            if (unlikely(physical == (void*)-1))
                return -EADDRNOTAVAIL;
            int err = vm_unmap_page(vm_ctx, virtual);
            if (err)
                return err;
            free_pages(physical, page_size_order);
            virtual = (u8*)virtual + page_size;
        }
    }

    if (free_virtual)
        free_vpages(saved_virtual, order);
    return 0;
}

void mmap_init(void) {
    struct limine_paging_mode_response* mode = g_limine_paging_mode_request.response;
    if (!mode) {
        u32 ecx, unused;
        cpuid(0x07, 0, &unused, &unused, &ecx, &unused);
        if (ecx & (1 << 16)) {
            u64 cr4;
            __asm__("movq %%cr4, %0" : "=r"(cr4));
            if (cr4 & CR4_LA57)
                panic("Wrong paging mode selected by the loader!");
        }
    } else if (mode->mode != LIMINE_PAGING_MODE_4LVL) {
        panic("Wrong paging mode selected by the loader!");
    }

    printk("Initialized virtual memory mapping\n");
}
