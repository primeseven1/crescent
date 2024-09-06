#include <crescent/common.h>
#include <crescent/asm/cpuid.h>
#include <crescent/asm/ctl.h>
#include <crescent/core/cpu.h>
#include <crescent/core/panic.h>
#include <crescent/mm/mmap.h>
#include <crescent/mm/hhdm.h>
#include <crescent/mm/zone.h>
#include <crescent/lib/string.h>

static inline unsigned long* get_table_virtaddr(unsigned long entry) {
    entry &= ~(0xFFF | MMU_FLAG_NX);
    return hhdm_virtual((unsigned long*)entry);
}

static inline unsigned long* alloc_table(void) {
    unsigned long* page = alloc_page(GFP_ZONE_NORMAL);
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

void* get_physaddr(struct vm_ctx* ctx, const void* virtaddr) {
    if (!is_virtaddr_canonical(virtaddr))
        return NULL;

    unsigned long* pml4, *pdpt, *pd, *pt;
    u16 pml4_i, pdpt_i, pd_i, pt_i;

    void* ret = NULL;

    unsigned long lock_flags;
    spinlock_lock_irq_save(&ctx->lock, &lock_flags);

    pml4 = ctx->ctx;
    pml4_i = ((uintptr_t)virtaddr >> 39) & 0x01FF;
    if (!(pml4[pml4_i] & MMU_FLAG_PRESENT))
        goto out;

    pdpt = get_table_virtaddr(pml4[pml4_i]);
    pdpt_i = ((uintptr_t)virtaddr >> 30) & 0x01FF;
    if (!(pdpt[pdpt_i] & MMU_FLAG_PRESENT))
        goto out;

    pd = get_table_virtaddr(pdpt[pdpt_i]);
    pd_i = ((uintptr_t)virtaddr >> 21) & 0x01FF;
    if (!(pd[pd_i] & MMU_FLAG_PRESENT))
        goto out;
    if (pd[pd_i] & MMU_FLAG_LARGE) {
        unsigned long offset = (uintptr_t)virtaddr - PAGE_ALIGN_2M(virtaddr);
        ret = (void*)((pd[pd_i] & ~(0xFFF | MMU_FLAG_NX)) + offset);
        goto out;
    }

    pt = get_table_virtaddr(pd[pd_i]);
    pt_i = ((uintptr_t)virtaddr >> 12) & 0x01FF;
    if (!(pt[pt_i] & MMU_FLAG_PRESENT))
        goto out;

    unsigned long offset = (uintptr_t)virtaddr - PAGE_ALIGN_4K(virtaddr);
    ret = (void*)((pt[pt_i] & ~(0xFFF | MMU_FLAG_NX)) + offset);
out:
    spinlock_unlock_irq_restore(&ctx->lock, &lock_flags);
    return ret;
}

int map_page(struct vm_ctx* ctx, void* virtaddr, void* physaddr, unsigned long mmu_flags) {
    if (!is_virtaddr_canonical(virtaddr) || !is_physaddr_canonical(physaddr))
        return -EFAULT;
    if (!is_flags_valid(mmu_flags))
        return -EINVAL;

    unsigned long* pml4, *pdpt, *pd, *pt; 
    u16 pml4_i, pdpt_i, pd_i, pt_i;

    /* Used for keeping track of what page tables that needed to be allocated */
    bool alloc_pdpt = false;
    bool alloc_pd = false;

    int err;

    unsigned long lock_flags;
    spinlock_lock_irq_save(&ctx->lock, &lock_flags);
    
    pml4 = ctx->ctx;
    pml4_i = ((uintptr_t)virtaddr >> 39) & 0x01FF;

    if (!(pml4[pml4_i] & MMU_FLAG_PRESENT)) {
        unsigned long* table = alloc_table();
        /* There are no resources to release, so just go to the out label */
        if (!table) {
            err = -ENOMEM;
            goto out;
        }
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
        err = 0;
        goto out;
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
            err = -ENOENT;
            goto err;
        }
        pd[pd_i] = (uintptr_t)table | MMU_FLAG_PRESENT | MMU_FLAG_READ_WRITE;
    }

    /* Now try mapping the 4K page */
    pt = get_table_virtaddr(pd[pd_i]);
    pt_i = ((uintptr_t)virtaddr >> 12) & 0x01FF;

    /* This can only happen if no page tables were allocated, so going to the out label is fine */
    if (pt[pt_i] & MMU_FLAG_PRESENT) {
        err = -EADDRINUSE;
        goto out;
    }

    /* Like with 2MiB pages, not aligning will cause problems with the mmu flags */
    physaddr = (void*)PAGE_ALIGN_4K(physaddr); 
    pt[pt_i] = (uintptr_t)physaddr | mmu_flags;
    _tlb_flush_single(virtaddr);
    err = 0;
    goto out;
err:
    if (alloc_pd) {
        free_table(pdpt[pdpt_i]);
        pdpt[pdpt_i] = 0;
    }
    if (alloc_pdpt) {
        free_table(pml4[pml4_i]);
        pml4[pml4_i] = 0;
    }
out:
    spinlock_unlock_irq_restore(&ctx->lock, &lock_flags);
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

int unmap_page(struct vm_ctx* ctx, void* virtaddr) {
    if (!is_virtaddr_canonical(virtaddr))
        return -EFAULT;

    unsigned long* pml4, *pdpt, *pd, *pt;
    u16 pml4_i, pdpt_i, pd_i, pt_i;

    int ret;

    unsigned long lock_flags;
    spinlock_lock_irq_save(&ctx->lock, &lock_flags);

    pml4 = ctx->ctx;
    pml4_i = ((uintptr_t)virtaddr) >> 39 & 0x01FF;
    if (!(pml4[pml4_i] & MMU_FLAG_PRESENT)) {
        ret = -ENOENT;
        goto out;
    }

    pdpt = get_table_virtaddr(pml4[pml4_i]);
    pdpt_i = ((uintptr_t)virtaddr >> 30) & 0x01FF;
    if (!(pdpt[pdpt_i] & MMU_FLAG_PRESENT)) {
        ret = -ENOENT;
        goto out;
    }

    /* Handle 2MiB pages */
    pd = get_table_virtaddr(pdpt[pdpt_i]);
    pd_i = ((uintptr_t)virtaddr >> 21) & 0x01FF;
    if (pd[pd_i] & MMU_FLAG_LARGE) {
        pd[pd_i] = 0;
        _tlb_flush_single(virtaddr);
        try_free_tables(pml4, pdpt, pml4_i, pd, pdpt_i, NULL, 0);
        ret = 0;
        goto out;
    }

    /* Page is a 4K page, continue checking */
    if (!(pd[pd_i] & MMU_FLAG_PRESENT)) {
        ret = -ENOENT;
        goto out;
    }

    pt = get_table_virtaddr(pd[pd_i]);
    pt_i = ((uintptr_t)virtaddr >> 12) & 0x01FF;

    pt[pt_i] = 0;
    _tlb_flush_single(virtaddr);
    try_free_tables(pml4, pdpt, pml4_i, pd, pdpt_i, pt, pd_i);
    ret = 0;
out:
    spinlock_unlock_irq_restore(&ctx->lock, &lock_flags);
    return ret;
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
}
