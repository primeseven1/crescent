#include <crescent/asm/paging.h>

static inline void tlb_flush_single(const void* vaddr)
{
    asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

int map_page(const void* vaddr, const void* paddr, int flags)
{
    u32** page_directory;
    asm volatile("movl %%cr3, %0" : "=r"(page_directory));
    page_directory = P2V(page_directory);
    unsigned long pd_index = (unsigned long)vaddr >> 22;

    /*
     * We would normally want to allocate a new page table,
     * but we don't have an allocator for that yet, so just do this there is one
     */
    if (!((uintptr_t)page_directory[pd_index] & 0x01))
        return -ERR_PDE_NOT_PRESENT;

    u32* page_table = (u32*)((uintptr_t)page_directory[pd_index] & ~0xFFF);
    page_table = P2V(page_table);
    unsigned long pt_index = (unsigned long)vaddr >> 12 & 0x03FF;

    if ((page_table[pt_index] & 0x01))
        return -ERR_PTE_PRESENT;

    page_table[pt_index] = ((uintptr_t)paddr | (flags & 0xFFF));
    tlb_flush_single(vaddr);

    return 0;
}

void unmap_page(const void* vaddr)
{
    u32** page_directory;
    asm volatile("movl %%cr3, %0" : "=r"(page_directory));
    page_directory = P2V(page_directory);
    unsigned long pd_index = (unsigned long)vaddr >> 22;

    u32* page_table = (u32*)((uintptr_t)page_directory[pd_index] & ~0xFFF);
    page_table = P2V(page_table);
    unsigned long pt_index = (unsigned long)vaddr >> 12 & 0x03FF;

    page_table[pt_index] = 0;
    tlb_flush_single(vaddr);
}

int map_pages(const void* vaddr, const void* paddr, int flags, unsigned int count)
{
    unsigned int pages_mapped = 0;

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

void unmap_pages(const void* vaddr, unsigned int count)
{
    while (count--) {
        unmap_page(vaddr);
        vaddr = (u8*)vaddr + PAGE_SIZE;
    }
}
