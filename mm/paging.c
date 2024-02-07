#include "crescent/error.h"
#include <crescent/asm/paging.h>

static inline void tlb_flush_single(const void* vaddr)
{
    asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

static inline pte_t** get_page_directory(void)
{
    pte_t** page_directory;
    asm volatile("movl %%cr3, %0" : "=r"(page_directory));
    page_directory = P2V(page_directory);
    return page_directory;
}

void* get_paddr(const void* vaddr)
{
    pte_t** page_directory = get_page_directory();
    unsigned long pd_index = (unsigned long)vaddr >> 22;

    if (!((uintptr_t)page_directory[pd_index] & 0x01))
        return NULL;

    pte_t* page_table = (pte_t*)((uintptr_t)page_directory[pd_index] & ~0xFFF);
    page_table = P2V(page_table);
    unsigned long pt_index = (unsigned long)vaddr >> 12 & 0x03FF;

    if (!(page_table[pt_index] & 0x01))
        return NULL;

    return (void*)((page_table[pt_index] & ~0xFFF) + ((unsigned long)vaddr & 0xFFF));
}

int map_page(const void* vaddr, const void* paddr, int flags)
{
    if ((uintptr_t)vaddr & 4095 || (uintptr_t)paddr & 4095)
        return -ERR_MISALIGNED_ADDR;

    pte_t** page_directory = get_page_directory();
    unsigned long pd_index = (unsigned long)vaddr >> 22;

    /*
     * We would normally want to allocate a new page table,
     * but we don't have an allocator for that yet, so just do this there is one
     */
    if (!((uintptr_t)page_directory[pd_index] & 0x01))
        return -ERR_PDE_NOT_PRESENT;

    pte_t* page_table = (pte_t*)((uintptr_t)page_directory[pd_index] & ~0xFFF);
    page_table = P2V(page_table);
    unsigned long pt_index = (unsigned long)vaddr >> 12 & 0x03FF;

    if (page_table[pt_index] & 0x01)
        return -ERR_PTE_PRESENT;

    page_table[pt_index] = ((uintptr_t)paddr | (flags & 0xFFF));
    tlb_flush_single(vaddr);

    return 0;
}

int unmap_page(const void* vaddr)
{
    if ((uintptr_t)vaddr & 4095)
        return -ERR_MISALIGNED_ADDR;

    pte_t** page_directory = get_page_directory();
    unsigned long pd_index = (unsigned long)vaddr >> 22;

    pte_t* page_table = (pte_t*)((uintptr_t)page_directory[pd_index] & ~0xFFF);
    page_table = P2V(page_table);
    unsigned long pt_index = (unsigned long)vaddr >> 12 & 0x03FF;

    page_table[pt_index] = 0;
    tlb_flush_single(vaddr);

    return 0;
}

int map_pages(const void* vaddr, const void* paddr, int flags, size_t count)
{
    size_t pages_mapped = 0;

    while (count--) {
        int err = map_page(vaddr, paddr, flags);
        if (err) {
            while (pages_mapped--) {
                vaddr = (u8*)vaddr - PAGE_SIZE;
                unmap_page(vaddr);
            }

            return err;
        }

        pages_mapped++;

        vaddr = (u8*)vaddr + PAGE_SIZE;
        paddr = (u8*)paddr + PAGE_SIZE;
    }

    return 0;
}

int unmap_pages(const void* vaddr, size_t count)
{
    /* 
     * Only need to check for an error on the first unmapping,
     * since the only return value for unmap_page is -ERR_MISALIGNED_ADDR
     */
    int err = unmap_page(vaddr);
    if (err)
        return err;

    vaddr = (u8*)vaddr + PAGE_SIZE;
    count--;

    while (count--) {
        unmap_page(vaddr);
        vaddr = (u8*)vaddr + PAGE_SIZE;
    }

    return 0;
}
