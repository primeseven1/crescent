#include <crescent/asm/paging.h>
#include <crescent/cpu/cpuid.h>

#define RECURSIVE_PML4_ENTRY 510

/*
 * These addresses are based on the recursive mapping happening at PML4 entry 510,
 * since 511 is taken by the kernel sections
 */
static pml4e_t* const pml4_vaddr = (pml4e_t*)0xFFFFFF7FBFDFE000;
static pdpte_t* const pdpt_vaddr = (pdpte_t*)0xFFFFFF7FBFC00000;
static pde_t* const pd_vaddr = (pde_t*)0xFFFFFF7F80000000;
static pte_t* const pt_vaddr = (pte_t*)0xFFFFFF0000000000;

static inline bool is_vaddr_canonical(const void* vaddr)
{
    /* Assuming a 48 bit virtual address space, which is safe to assume */ 
    const void* non_canonical_start = (void*)0x800000000000;
    const void* non_canonical_end = (void*)0xFFFF7FFFFFFFFFFF;

    return (vaddr < non_canonical_start || vaddr > non_canonical_end);
}

static inline bool is_paddr_supported(const void* paddr)
{
    static void* max_ptr = NULL;

    if (unlikely(!max_ptr)) {
        /* First get the amount of supported bits using cpuid */
        u32 eax, ebx, ecx, edx;
        cpuid_all(0x80000008, 0, &eax, &ebx, &ecx, &edx);
        int bits = eax & 0xFF;
        /* Now get the maximum integer for the amount the CPU supports */
        max_ptr = (void*)((1ull << bits) - 1);
    }

    return (paddr <= max_ptr);
}

static inline bool is_flags_good(u64 flags)
{
    /* Unset the page flags, if any bits are still set, the flags are invalid */
    flags &= ~(0xFFF | PT_EXECUTE_DISABLE);
    return !flags;
}

void* get_paddr(const void* vaddr)
{
    if (!is_vaddr_canonical(vaddr))
        return NULL;

    int offset = (uintptr_t)vaddr - (uintptr_t)PAGE_ALIGN(vaddr);
    vaddr = PAGE_ALIGN(vaddr);

    u64 pml4_i = ((uintptr_t)vaddr >> 39) & 0x01FF;
    u64 pdpt_i = ((uintptr_t)vaddr >> 30) & 0x3FFFF;
    u64 pd_i = ((uintptr_t)vaddr >> 21) & 0x7FFFFFF;
    u64 pt_i = ((uintptr_t)vaddr >> 12) & 0xFFFFFFFFF;

    if (!(pml4_vaddr[pml4_i] & PT_PRESENT))
        return NULL;
    if (!(pdpt_vaddr[pdpt_i] & PT_PRESENT))
        return NULL;
    if (!(pd_vaddr[pd_i] & PT_PRESENT))
        return NULL;
    if (!(pt_vaddr[pt_i] & PT_PRESENT))
        return NULL;

    /* Now clear the page table flags, and add the offset to get the virtual address */
    return (void*)((pt_vaddr[pt_i] & (~0xFFF | PT_EXECUTE_DISABLE)) + offset);
}

int map_page_table(void* vaddr, const generic_pte_t* pt_paddr, u64 flags, unsigned int level)
{
    if (!is_paddr_supported(pt_paddr))
        return -E2BIG;
    if (!is_flags_good(flags) || !is_vaddr_canonical(vaddr) || (uintptr_t)pt_paddr & 4095)
        return -EINVAL;

    vaddr = PAGE_ALIGN(vaddr);

    u64 pml4_i = ((uintptr_t)vaddr >> 39) & 0x01FF;
    u64 pdpt_i = ((uintptr_t)vaddr >> 30) & 0x3FFFF;
    u64 pd_i = ((uintptr_t)vaddr >> 21) & 0x7FFFFFF;

    if (pml4_i == RECURSIVE_PML4_ENTRY)
        return -EBUSY;

    /* 
     * The reason present and read/write is forced is because writing to the page 
     * tables would cause a page fault without the flags 
     */
    switch (level) {
    case PT_LEVEL_PT:
        if (!(pml4_vaddr[pml4_i] & PT_PRESENT))
            return -EADDRNOTAVAIL;
        if (!(pdpt_vaddr[pdpt_i] & PT_PRESENT))
            return -EADDRNOTAVAIL;
        if (pd_vaddr[pd_i] & PT_PRESENT)
            return -EBUSY;
        pd_vaddr[pd_i] = (uintptr_t)pt_paddr | (flags | PT_PRESENT | PT_READ_WRITE);
        break;
    case PT_LEVEL_PD:
        if (!(pml4_vaddr[pml4_i] & PT_PRESENT))
            return -EADDRNOTAVAIL;
        if (pdpt_vaddr[pdpt_i] & PT_PRESENT)
            return -EBUSY;
        pdpt_vaddr[pdpt_i] = (uintptr_t)pt_paddr | (flags | PT_PRESENT | PT_READ_WRITE);
        break;
    case PT_LEVEL_PDPT:
        if (pml4_vaddr[pml4_i] & PT_PRESENT)
            return -EBUSY;
        pml4_vaddr[pml4_i] = (uintptr_t)pt_paddr | (flags | PT_PRESENT | PT_READ_WRITE);
        break;
    default:
        return -EINVAL;
    }

    tlb_flush_all();

    return 0;
}

int unmap_page_table(void* vaddr, unsigned int level)
{
    if (!is_vaddr_canonical(vaddr))
        return -EINVAL;

    vaddr = PAGE_ALIGN(vaddr);

    u64 pml4_i = ((uintptr_t)vaddr >> 39) & 0x01FF;
    u64 pdpt_i = ((uintptr_t)vaddr >> 30) & 0x3FFFF;
    u64 pd_i = ((uintptr_t)vaddr >> 21) & 0x7FFFFFF;

    if (pml4_i == RECURSIVE_PML4_ENTRY)
        return -EBUSY;

    switch (level) {
    case PT_LEVEL_PT:
        if (!(pml4_vaddr[pml4_i] & PT_PRESENT))
            return -EADDRNOTAVAIL;
        if (!(pdpt_vaddr[pdpt_i] & PT_PRESENT))
            return -EADDRNOTAVAIL;
        pd_vaddr[pd_i] = 0;
        break;
    case PT_LEVEL_PD:
        if (!(pml4_vaddr[pml4_i] & PT_PRESENT))
            return -EADDRNOTAVAIL;
        pdpt_vaddr[pdpt_i] = 0;
        break;
    case PT_LEVEL_PDPT:
        pml4_vaddr[pml4_i] = 0;
        break;
    default:
        return -EINVAL;
    }

    tlb_flush_all();

    return 0;
}

int map_page(void* vaddr, const void* paddr, u64 flags)
{
    if (!is_paddr_supported(paddr))
        return -E2BIG;
    if (!is_vaddr_canonical(vaddr) || !is_flags_good(flags))
        return -EINVAL;

    vaddr = PAGE_ALIGN(vaddr);
    paddr = PAGE_ALIGN(paddr);

    u64 pml4_i = ((uintptr_t)vaddr >> 39) & 0x01FF;
    u64 pdpt_i = ((uintptr_t)vaddr >> 30) & 0x3FFFF;
    u64 pd_i = ((uintptr_t)vaddr >> 21) & 0x7FFFFFF;
    u64 pt_i = ((uintptr_t)vaddr >> 12) & 0xFFFFFFFFF;

    if (pml4_i == RECURSIVE_PML4_ENTRY)
        return -EBUSY;

    if (!(pml4_vaddr[pml4_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;
    if (!(pdpt_vaddr[pdpt_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;
    if (!(pd_vaddr[pd_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;

    if (pt_vaddr[pt_i] & PT_PRESENT)
        return -EBUSY;

    pt_vaddr[pt_i] = (uintptr_t)paddr | flags;
    tlb_flush_single(vaddr);

    return 0;
}

int unmap_page(void* vaddr)
{
    if (!is_vaddr_canonical(vaddr))
        return -EINVAL;

    vaddr = PAGE_ALIGN(vaddr);

    u64 pml4_i = ((uintptr_t)vaddr >> 39) & 0x01FF;
    u64 pdpt_i = ((uintptr_t)vaddr >> 30) & 0x3FFFF;
    u64 pd_i = ((uintptr_t)vaddr >> 21) & 0x7FFFFFF;
    u64 pt_i = ((uintptr_t)vaddr >> 12) & 0xFFFFFFFFF;

    if (pml4_i == RECURSIVE_PML4_ENTRY)
        return -EBUSY;

    if (!(pml4_vaddr[pml4_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;
    if (!(pdpt_vaddr[pdpt_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;
    if (!(pd_vaddr[pd_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;

    pt_vaddr[pt_i] = 0;
    tlb_flush_single(vaddr);

    return 0;
}

int map_pages(void* vaddr, const void* paddr, u64 flags, size_t count)
{
    size_t pages_mapped = 0;

    while (count--) {
        int err = map_page(vaddr, paddr, flags);
        if (err) {
            /* Unmap all pages that were previously mapped, for security reasons */
            while (pages_mapped--) {
                vaddr = (u8*)vaddr - PAGE_SIZE;
                paddr = (u8*)paddr - PAGE_SIZE;

                unmap_page(vaddr);
            }

            return err;
        }

        vaddr = (u8*)vaddr + PAGE_SIZE;
        paddr = (u8*)paddr + PAGE_SIZE;

        pages_mapped++;
    }

    return 0; 
}

int unmap_pages(void* vaddr, size_t count)
{
    while (count--) {
        int err = unmap_page(vaddr);
        if (err)
            return err; 
        vaddr = (u8*)vaddr + PAGE_SIZE;
    }

    return 0;
}
