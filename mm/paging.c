#include <crescent/asm/paging.h>
#include <crescent/cpu/cpuid.h>

static inline void* get_cr3(void)
{
    void* ret;
    asm volatile("movq %%cr3, %0" : "=r"(ret));
    return P2V(ret);
}

static inline bool is_vaddr_good(const void* vaddr)
{
    /* Here we only really need to check if the address is canonical */
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
    if (!is_vaddr_good(vaddr))
        return NULL;

    int pml4_i = (uintptr_t)vaddr >> 39 & 0x01FF;
    int pdpt_i = (uintptr_t)vaddr >> 30 & 0x01FF;
    int pd_i = (uintptr_t)vaddr >> 21 & 0x01FF;
    int pt_i = (uintptr_t)vaddr >> 12 & 0x01FF;

    pte_t**** pml4 = get_cr3();
    if (!((pml4e_t)pml4[pml4_i] & 0x01))
        return NULL;
    pte_t*** pdpt = P2V(((pdpte_t)pml4[pml4_i] & ~(0xFFF | PT_EXECUTE_DISABLE)));
    if (!((pdpte_t)pdpt[pdpt_i] & 0x01))
        return NULL;
    pte_t** pd = P2V(((pde_t)pdpt[pdpt_i] & ~(0xFFF | PT_EXECUTE_DISABLE)));
    if (!((pde_t)pd[pd_i] & 0x01))
        return NULL;
    pte_t* pt = P2V(((pte_t)pd[pd_i] & ~(0xFFF | PT_EXECUTE_DISABLE)));
    if (!(pt[pt_i] & 0x01))
        return NULL;

    int offset = (uintptr_t)vaddr - (uintptr_t)PAGE_ALIGN(vaddr);
    return (void*)((pt[pt_i] & (~0xFFF | PT_EXECUTE_DISABLE)) + offset);
}

int map_page_table(void* vaddr, const pte_t* pt_paddr, u64 flags)
{
    if (!is_paddr_good(pt_paddr))
        return -E2BIG;
    if (!is_flags_good(flags) || !is_vaddr_good(vaddr) || (uintptr_t)pt_paddr & 4095)
        return -EINVAL;

    vaddr = PAGE_ALIGN(vaddr);

    int pml4_i = (uintptr_t)vaddr >> 39 & 0x01FF;
    int pdpt_i = (uintptr_t)vaddr >> 30 & 0x01FF;
    int pd_i = (uintptr_t)vaddr >> 21 & 0x01FF;

    pde_t*** pml4 = get_cr3();
    if (!((pml4e_t)pml4[pml4_i] & 0x01))
        return -EADDRNOTAVAIL;
    pde_t** pdpt = P2V(((pdpte_t)pml4[pml4_i] & ~(0xFFF | PT_EXECUTE_DISABLE)));
    if (!((pdpte_t)pdpt[pdpt_i] & 0x01))
        return -EADDRNOTAVAIL;

    /* Remapping a whole page table while one is still present can be very bad */
    pde_t* pd = P2V(((pde_t)pdpt[pdpt_i] & ~(0xFFF | PT_EXECUTE_DISABLE)));
    if (((pde_t)pd[pd_i] & 0x01))
        return -EBUSY;

    pd[pd_i] = (uintptr_t)pt_paddr | flags;
    tlb_flush_all();

    return 0;
}

int unmap_page_table(void* vaddr)
{
    if (!is_vaddr_good(vaddr))
        return -EINVAL;

    vaddr = PAGE_ALIGN(vaddr);

    int pml4_i = (uintptr_t)vaddr >> 39 & 0x01FF;
    int pdpt_i = (uintptr_t)vaddr >> 30 & 0x01FF;
    int pd_i = (uintptr_t)vaddr >> 21 & 0x01FF;

    pde_t*** pml4 = get_cr3();
    if (!((pml4e_t)pml4[pml4_i] & 0x01))
        return -EADDRNOTAVAIL;
    pde_t** pdpt = P2V(((pdpte_t)pml4[pml4_i] & ~(0xFFF | PT_EXECUTE_DISABLE)));
    if (!((pdpte_t)pdpt[pdpt_i] & 0x01))
        return -EADDRNOTAVAIL;

    pde_t* pd = P2V(((pde_t)pdpt[pdpt_i] & ~(0xFFF | PT_EXECUTE_DISABLE)));
    pd[pd_i] = 0;

    tlb_flush_all();

    return 0;
}

int map_page(void* vaddr, const void* paddr, u64 flags)
{
    if (!is_paddr_good(paddr))
        return -E2BIG;
    if (!is_vaddr_good(vaddr) || !is_flags_good(flags))
        return -EINVAL;

    vaddr = PAGE_ALIGN(vaddr);
    paddr = PAGE_ALIGN(paddr);

    int pml4_i = (uintptr_t)vaddr >> 39 & 0x01FF;
    int pdpt_i = (uintptr_t)vaddr >> 30 & 0x01FF;
    int pd_i = (uintptr_t)vaddr >> 21 & 0x01FF;
    int pt_i = (uintptr_t)vaddr >> 12 & 0x01FF;

    /* First time i've had to use a quadruple pointer for a practical reason... */
    pte_t**** pml4 = get_cr3();
    if (!((pml4e_t)pml4[pml4_i] & 0x01))
        return -EADDRNOTAVAIL;
    pte_t*** pdpt = P2V(((pdpte_t)pml4[pml4_i] & ~(0xFFF | PT_EXECUTE_DISABLE)));
    if (!((pdpte_t)pdpt[pdpt_i] & 0x01))
        return -EADDRNOTAVAIL;
    pte_t** pd = P2V(((pde_t)pdpt[pdpt_i] & ~(0xFFF | PT_EXECUTE_DISABLE)));
    if (!((pde_t)pd[pd_i] & 0x01))
        return -EADDRNOTAVAIL;

    pte_t* pt = P2V(((pte_t)pd[pd_i] & ~(0xFFF | PT_EXECUTE_DISABLE)));
    if (pt[pt_i] & 0x01)
        return -EBUSY;

    pt[pt_i] = (uintptr_t)paddr | flags;
    tlb_flush_single(vaddr);

    return 0;
}

int unmap_page(void* vaddr)
{
    if (!is_vaddr_good(vaddr))
        return -EINVAL;

    vaddr = PAGE_ALIGN(vaddr);

    int pml4_i = (uintptr_t)vaddr >> 39 & 0x01FF;
    int pdpt_i = (uintptr_t)vaddr >> 30 & 0x01FF;
    int pd_i = (uintptr_t)vaddr >> 21 & 0x01FF;
    int pt_i = (uintptr_t)vaddr >> 12 & 0x01FF;

    /* Here we go again with quadruple pointers */
    pte_t**** pml4 = get_cr3();
    if (!((pml4e_t)pml4[pml4_i] & 0x01))
        return -EADDRNOTAVAIL;
    pte_t*** pdpt = P2V(((pdpte_t)pml4[pml4_i] & ~(0xFFF | PT_EXECUTE_DISABLE)));
    if (!((pdpte_t)pdpt[pdpt_i] & 0x01))
        return -EADDRNOTAVAIL;
    pte_t** pd = P2V(((pde_t)pdpt[pdpt_i] & ~(0xFFF | PT_EXECUTE_DISABLE)));
    if (!((pde_t)pd[pd_i] & 0x01))
        return -EADDRNOTAVAIL;

    pte_t* pt = P2V(((pte_t)pd[pd_i] & ~(0xFFF | PT_EXECUTE_DISABLE)));
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
