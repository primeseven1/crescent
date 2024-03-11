#include <crescent/asm/paging.h>
#include <crescent/cpu/cpuid.h>

#define RECURSIVE_PML4_ENTRY 510

/*
 * These addresses are based on the recursive mapping happening at PML4 entry 510,
 * since 511 is taken by the kernel sections
 *
 * Since these functions interpret the page table entries differently depending on the context,
 * using generic_pte_t is used here
 */
static generic_pte_t* const pml4_vaddr = (generic_pte_t*)0xFFFFFF7FBFDFE000;
static generic_pte_t* const pdpt_vaddr = (generic_pte_t*)0xFFFFFF7FBFC00000;
static generic_pte_t* const pd_vaddr = (generic_pte_t*)0xFFFFFF7F80000000;
static generic_pte_t* const pt_vaddr = (generic_pte_t*)0xFFFFFF0000000000;

static inline bool is_vaddr_canonical(const void* vaddr)
{
    return ((uintptr_t)vaddr >> 48 == 0 || (uintptr_t)vaddr >> 48 == 0xFFFF);
}

static inline bool is_paddr_good(const void* paddr)
{
    static int num_paddr_bits = 0;
    if (unlikely(!num_paddr_bits)) {
        u32 eax, ebx, ecx, edx;
        cpuid_all(0x80000008, 0, &eax, &ebx, &ecx, &edx);
        num_paddr_bits = eax & 0xFF;
    }

    /* Make sure that the bits above the supported amount of bits are not set */
    return (uintptr_t)paddr >> num_paddr_bits == 0;
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

    pte_t**** pml4 = (pte_t****)pml4_vaddr;
    if (!((pml4e_t)pml4[pml4_i] & PT_PRESENT))
        return NULL;
    pte_t*** pdpt = (pte_t***)pdpt_vaddr;
    if (!((pdpte_t)pdpt[pdpt_i] & PT_PRESENT))
        return NULL;
    pte_t** pd = (pte_t**)pd_vaddr;
    if (!((pde_t)pd[pd_i] & PT_PRESENT))
        return NULL;
    pte_t* pt = (pte_t*)pt_vaddr;
    if (!(pt[pt_i] & PT_PRESENT))
        return NULL;

    return (void*)((pt[pt_i] & (~0xFFF | PT_EXECUTE_DISABLE)) + offset);
}

int map_page_table(void* vaddr, const pte_t* pt_paddr, u64 flags)
{
    if (!is_paddr_good(pt_paddr))
        return -E2BIG;
    if (!is_flags_good(flags) || !is_vaddr_canonical(vaddr) || (uintptr_t)pt_paddr & 4095)
        return -EINVAL;

    vaddr = PAGE_TABLE_ALIGN(vaddr);

    u64 pml4_i = ((uintptr_t)vaddr >> 39) & 0x01FF;
    u64 pdpt_i = ((uintptr_t)vaddr >> 30) & 0x3FFFF;
    u64 pd_i = ((uintptr_t)vaddr >> 21) & 0x7FFFFFF;

    if (pml4_i == RECURSIVE_PML4_ENTRY)
        return -EBUSY;

    pde_t*** pml4 = (pde_t***)pml4_vaddr;
    if (!((pml4e_t)pml4[pml4_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;
    pde_t** pdpt = (pde_t**)pdpt_vaddr;
    if (!((pdpte_t)pdpt[pdpt_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;

    /* Remapping a whole page table while one is still present can be very bad */
    pde_t* pd = (pde_t*)pd_vaddr;
    if (((pde_t)pd[pd_i] & PT_PRESENT))
        return -EBUSY;

    pd[pd_i] = (uintptr_t)pt_paddr | (flags | PT_PRESENT | PT_READ_WRITE);
    tlb_flush_all();

    return 0;
}

int unmap_page_table(void* vaddr)
{
    if (!is_vaddr_canonical(vaddr))
        return -EINVAL;

    vaddr = PAGE_TABLE_ALIGN(vaddr);

    u64 pml4_i = ((uintptr_t)vaddr >> 39) & 0x01FF;
    u64 pdpt_i = ((uintptr_t)vaddr >> 30) & 0x3FFFF;
    u64 pd_i = ((uintptr_t)vaddr >> 21) & 0x7FFFFFF;

    if (pml4_i == RECURSIVE_PML4_ENTRY)
        return -EBUSY;

    pde_t*** pml4 = (pde_t***)pml4_vaddr;
    if (!((pml4e_t)pml4[pml4_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;
    pde_t** pdpt = (pde_t**)pdpt_vaddr;
    if (!((pdpte_t)pdpt[pdpt_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;

    pde_t* pd = pd_vaddr;
    pd[pd_i] = 0;

    tlb_flush_all();

    return 0;
}

int map_page(void* vaddr, const void* paddr, u64 flags)
{
    if (!is_paddr_good(paddr))
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

    /* First time i've had to use a quadruple pointer for a practical reason... */
    pte_t**** pml4 = (pte_t****)pml4_vaddr;
    if (!((pml4e_t)pml4[pml4_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;
    pte_t*** pdpt = (pte_t***)pdpt_vaddr;
    if (!((pdpte_t)pdpt[pdpt_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;
    pte_t** pd = (pte_t**)pd_vaddr;
    if (!((pde_t)pd[pd_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;

    pte_t* pt = (pte_t*)pt_vaddr;
    if (pt[pt_i] & PT_PRESENT)
        return -EBUSY;

    pt[pt_i] = (uintptr_t)paddr | flags;
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

    /* Here we go again with quadruple pointers */
    pte_t**** pml4 = (pte_t****)pml4_vaddr;
    if (!((pml4e_t)pml4[pml4_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;
    pte_t*** pdpt = (pte_t***)pdpt_vaddr; 
    if (!((pdpte_t)pdpt[pdpt_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;
    pte_t** pd = (pte_t**)pd_vaddr;
    if (!((pde_t)pd[pd_i] & PT_PRESENT))
        return -EADDRNOTAVAIL;

    pte_t* pt = (pte_t*)pt_vaddr;
    pt[pt_i] = 0;
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
