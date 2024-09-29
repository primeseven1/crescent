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
    entry &= ~(0xFFF | PT_NX);
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
    entry &= ~(0xFFF | PT_NX);
    free_page((unsigned long*)entry);
}

/* Attempt to free page tables once a page is unmapped */
static void try_free_tables(unsigned long* pml4, unsigned long* pdpt, 
        u16 pml4_i, unsigned long* pd, u16 pdpt_i, unsigned long* pt, u16 pd_i) {
    /* pt is null if a huge page is unmapped */
    if (pt) {
        for (u16 i = 0; i < 512; i++) {
            if (pt[i] & PT_PRESENT)
                return;
        }

        free_table((uintptr_t)hhdm_physical(pt));
        pd[pd_i] = 0;
    }

    for (u16 i = 0; i < 512; i++) {
        if (pd[i] & PT_PRESENT)
            return;
    }

    free_table((uintptr_t)hhdm_physical(pd));
    pdpt[pdpt_i] = 0;

    for (u16 i = 0; i < 512; i++) {
        if (pdpt[i] & PT_PRESENT)
            return;
    }

    free_table((uintptr_t)hhdm_physical(pdpt));
    pml4[pml4_i] = 0;
}

static inline bool is_flags_valid(unsigned long flags) {
    flags &= ~(0xFFF | PT_NX);
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
    if (!(pml4[pml4_i] & PT_PRESENT))
        return false;

    pdpt = get_table_virtaddr(pml4[pml4_i]);
    pdpt_i = ((uintptr_t)virtaddr >> 30) & 0x01FF;
    if (!(pdpt[pdpt_i] & PT_PRESENT))
        return false;

    pd = get_table_virtaddr(pdpt[pdpt_i]);
    pd_i = ((uintptr_t)virtaddr >> 21) & 0x01FF;
    if (!(pd[pd_i] & PT_PRESENT))
        return false;
    if (pd[pd_i] & PT_LARGE)
        return true;

    return false;
}

static void* get_physaddr(struct vm_ctx* ctx, const void* virtaddr) {
    unsigned long* pml4, *pdpt, *pd, *pt;
    u16 pml4_i, pdpt_i, pd_i, pt_i;

    pml4 = ctx->ctx;
    pml4_i = ((uintptr_t)virtaddr >> 39) & 0x01FF;
    if (!(pml4[pml4_i] & PT_PRESENT))
        return NULL;

    pdpt = get_table_virtaddr(pml4[pml4_i]);
    pdpt_i = ((uintptr_t)virtaddr >> 30) & 0x01FF;
    if (!(pdpt[pdpt_i] & PT_PRESENT))
        return NULL;

    pd = get_table_virtaddr(pdpt[pdpt_i]);
    pd_i = ((uintptr_t)virtaddr >> 21) & 0x01FF;
    if (!(pd[pd_i] & PT_PRESENT))
        return NULL;
    if (pd[pd_i] & PT_LARGE) {
        unsigned long offset = (uintptr_t)virtaddr - PAGE_ALIGN_2M(virtaddr);
        return (void*)((pd[pd_i] & ~(0xFFF | PT_NX)) + offset);
    }

    pt = get_table_virtaddr(pd[pd_i]);
    pt_i = ((uintptr_t)virtaddr >> 12) & 0x01FF;
    if (!(pt[pt_i] & PT_PRESENT))
        return NULL;

    unsigned long offset = (uintptr_t)virtaddr - PAGE_ALIGN_4K(virtaddr);
    return (void*)((pt[pt_i] & ~(0xFFF | PT_NX)) + offset);
}

static int map_page(struct vm_ctx* ctx, void* virtaddr, void* physaddr, unsigned long pt_flags) {
    unsigned long* pml4, *pdpt, *pd, *pt; 
    u16 pml4_i, pdpt_i, pd_i, pt_i;

    /* Used for keeping track of what page tables that needed to be allocated */
    bool alloc_pdpt = false;
    bool alloc_pd = false;

    int err;
 
    pml4 = ctx->ctx;
    pml4_i = ((uintptr_t)virtaddr >> 39) & 0x01FF;

    if (!(pml4[pml4_i] & PT_PRESENT)) {
        unsigned long* table = alloc_table();
        /* There are no resources to release, so just return */
        if (!table)
            return -ENOMEM;

        alloc_pdpt = true;
        pml4[pml4_i] = (uintptr_t)table | PT_PRESENT | PT_READ_WRITE;
    }

    pdpt = get_table_virtaddr(pml4[pml4_i]);
    pdpt_i = ((uintptr_t)virtaddr >> 30) & 0x01FF;

    if (!(pdpt[pdpt_i] & PT_PRESENT)) {
        unsigned long* table = alloc_table();
        if (!table) {
            err = -ENOMEM;
            goto err;
        }
        alloc_pd = true;
        pdpt[pdpt_i] = (uintptr_t)table | PT_PRESENT | PT_READ_WRITE;
    }

    pd = get_table_virtaddr(pdpt[pdpt_i]);
    pd_i = ((uintptr_t)virtaddr >> 21) & 0x01FF;

    if (pt_flags & PT_LARGE) {
        if (pd[pd_i] & PT_PRESENT) {
            err = -EADDRINUSE;
            goto err;
        }

        /* Not aligning the address will cause problems with the mmu flags */
        physaddr = (void*)PAGE_ALIGN_2M(physaddr);
        pd[pd_i] = (uintptr_t)physaddr | pt_flags;
        _tlb_flush_single(virtaddr);
        return 0;
    }

    /* 
     * Make sure that the PD does not have the huge page flag, 
     * since the PD maps directly to the physical address when set 
     */
    if (pd[pd_i] & PT_LARGE) {
        err = -ERANGE;
        goto err;
    } else if (!(pd[pd_i] & PT_PRESENT)) {
        unsigned long* table = alloc_table();
        if (!table) {
            err = -ENOMEM;
            goto err;
        }
        pd[pd_i] = (uintptr_t)table | PT_PRESENT | PT_READ_WRITE;
    }

    /* Now try mapping the 4K page */
    pt = get_table_virtaddr(pd[pd_i]);
    pt_i = ((uintptr_t)virtaddr >> 12) & 0x01FF;

    /* This can only happen if no page tables were allocated, so returning is fine */
    if (pt[pt_i] & PT_PRESENT)
        return -EADDRINUSE;

    /* Like with 2MiB pages, not aligning will cause problems with the mmu flags */
    physaddr = (void*)PAGE_ALIGN_4K(physaddr); 
    pt[pt_i] = (uintptr_t)physaddr | pt_flags;
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

static int unmap_page(struct vm_ctx* ctx, void* virtaddr) {
    unsigned long* pml4, *pdpt, *pd, *pt;
    u16 pml4_i, pdpt_i, pd_i, pt_i;

    pml4 = ctx->ctx;
    pml4_i = ((uintptr_t)virtaddr) >> 39 & 0x01FF;
    if (!(pml4[pml4_i] & PT_PRESENT))
        return -ENOENT;

    pdpt = get_table_virtaddr(pml4[pml4_i]);
    pdpt_i = ((uintptr_t)virtaddr >> 30) & 0x01FF;
    if (!(pdpt[pdpt_i] & PT_PRESENT))
        return -ENOENT;

    /* Handle 2MiB pages */
    pd = get_table_virtaddr(pdpt[pdpt_i]);
    pd_i = ((uintptr_t)virtaddr >> 21) & 0x01FF;
    if (pd[pd_i] & PT_LARGE) {
        pd[pd_i] = 0;
        _tlb_flush_single(virtaddr);
        try_free_tables(pml4, pdpt, pml4_i, pd, pdpt_i, NULL, 0);
        return 0;
    }

    /* Page is a 4K page, continue checking */
    if (!(pd[pd_i] & PT_PRESENT))
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
        return NULL;

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

static int vm_map_pages(struct vm_ctx* ctx, void* virtual, 
        void* physical, unsigned long pt_flags, unsigned long count) {
    if (!is_virtaddr_canonical(virtual) || !is_physaddr_canonical(physical))
        return -EFAULT;
    if (!is_flags_valid(pt_flags))
        return -EINVAL;

    int err = 0;
    unsigned long lock_flags;
    size_t page_size = pt_flags & PT_LARGE ? 0x200000 : 0x1000;

    u16 top_lvl_entry = (uintptr_t)virtual >> 39 & 0x01FF;
    if (top_lvl_entry >= 256)
        spinlock_lock_irq_save(&kas_lock, &lock_flags);
    else
        spinlock_lock_irq_save(&ctx->lock, &lock_flags);

    unsigned long pages_mapped = 0;
    while (count--) {
        err = map_page(ctx, virtual, physical, pt_flags);
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

static unsigned long mmu_to_pt(unsigned long mmu_flags) {
    if (!(mmu_flags & MMU_READ))
        return 0;
    if (mmu_flags & MMU_WRITE_THROUGH && mmu_flags & MMU_CACHE_DISABLE)
        return 0;

    /* 
     * Most of the MMU flags are the same as the PT flags for 
     * convenience reasons, NX is not one of them
     */
    if (!(mmu_flags & MMU_EXEC))
        mmu_flags |= PT_NX;
    return mmu_flags;
}

void* kmmap(void* virtual, size_t size, unsigned int flags, 
        unsigned long mmu_flags, gfp_t gfp_flags, void* private, int* errno) {
    unsigned long pt_flags = mmu_to_pt(mmu_flags);
    if (size == 0 || pt_flags == 0) {
        *errno = -EINVAL;
        return NULL;
    }

    size_t page_size;
    if (gfp_flags & GFP_VM_LARGE_PAGE) {
        page_size = 0x200000;
        pt_flags |= PT_LARGE;
    } else {
        page_size = 0x1000;
    }

    unsigned long page_count = size & (page_size - 1) ? size / page_size + 1 : size / page_size;

    gfp_t gfp_virt = gfp_flags & GFP_VM_FLAGS_MASK;
    unsigned int vorder = get_order(size);
    virtual = alloc_vpages(gfp_virt, vorder);
    if (!virtual) {
        *errno = -ENOMEM;
        return NULL;
    }

    struct vm_ctx* vm_ctx = &_cpu()->vm_ctx;
    void* saved_virtual = virtual;

    if (flags & KMMAP_PHYS) {
        *errno = vm_map_pages(vm_ctx, virtual, private, pt_flags, page_count);
        if (*errno == 0)
            return virtual;
    } else if (flags & KMMAP_ALLOC) {
        gfp_t gfp_phys = gfp_flags & GFP_PM_FLAGS_MASK;
        unsigned int page_size_order = get_order(page_size); 

        unsigned long pages_mapped = 0;
        while (page_count) {
            void* physical = alloc_pages(gfp_phys, page_size_order);
            if (!physical) {
                *errno = -ENOMEM;
                break;
            }

            *errno = vm_map_pages(vm_ctx, virtual, physical, pt_flags, 1);
            if (*errno)
                break;

            page_count--;
            pages_mapped++;

            virtual = (u8*)virtual + page_size;
        }

        if (page_count == 0)
            return saved_virtual;

        while (pages_mapped--) {
            virtual = (u8*)virtual - page_size;
            void* physical = vm_get_physaddr(vm_ctx, virtual);
            if (unlikely(!physical)) {
                printk(PL_ERR "vm_get_physaddr returned NULL in error handling\n");
                continue;
            }

            free_pages(physical, page_size_order);
            vm_unmap_pages(vm_ctx, virtual, 1);
        }
    }

    free_vpages(saved_virtual, vorder);
    return NULL;
}

int kmunmap(void* virtual, size_t size, unsigned int flags) {
    if (!virtual)
        return -EFAULT;
    if (size == 0)
        return -EINVAL;

    struct vm_ctx* vm_ctx = &_cpu()->vm_ctx;
    unsigned int order = get_order(size);

    size_t page_size = is_huge_page(vm_ctx, virtual) ? 0x200000 : 0x1000;
    unsigned int page_size_order = get_order(page_size);
    size_t page_count = size & (page_size - 1) ? size / page_size + 1 : size / page_size;

    if (flags & KMMAP_PHYS) {
        int err = vm_unmap_pages(vm_ctx, virtual, page_count);
        if (err)
            return err;
    } else if (flags & KMMAP_ALLOC) {
        while (page_count--) {
            void* physical = vm_get_physaddr(vm_ctx, virtual);
            if (unlikely(!physical))
                return -EADDRNOTAVAIL;
            int err = vm_unmap_pages(vm_ctx, virtual, 1);
            if (err)
                return err;
            free_pages(physical, page_size_order);
            virtual = (u8*)virtual + page_size;
        }
    }

    free_vpages(virtual, order);
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
