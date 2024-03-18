#include <crescent/kernel.h>
#include <crescent/mmap.h>

static const struct multiboot_tag_mmap* multiboot_mmap = NULL;

size_t get_total_mmap_free_memory(void)
{
    if (unlikely(!multiboot_mmap))
        return 0;

    size_t total_mem = 0;

    const struct multiboot_mmap_entry* entry = multiboot_mmap->entries;
    while ((u8*)entry < (u8*)multiboot_mmap + multiboot_mmap->tag.size) {
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) 
            total_mem += entry->len;

        entry = (struct multiboot_mmap_entry*)((u8*)entry + multiboot_mmap->entry_size);
    }

    return total_mem;
}

bool is_paddr_mmap_region_free(void* paddr, size_t size)
{
    if (unlikely(!multiboot_mmap))
        return false;
    if (!size)
        size = 1;

    u64 target_start = (u64)paddr;
    u64 target_end = (u64)paddr + size - 1;

    bool ret = false;

    const struct multiboot_mmap_entry* entry = multiboot_mmap->entries;
    while ((u8*)entry < (u8*)multiboot_mmap + multiboot_mmap->tag.size) {
        u64 entry_start = entry->addr;
        u64 entry_end = entry->addr + entry->len - 1;

        /* 
         * Cannot return right away if it's free because there could be a region that overlaps.
         * The memory map is also probably out of order too, because some BIOS/UEFI implimentations suck
         */
        if (target_start <= entry_end && target_end >= entry_start) {
            if (entry->type != MULTIBOOT_MEMORY_AVAILABLE)
                return false;
           
            ret = true;
        }

        entry = (struct multiboot_mmap_entry*)((u8*)entry + multiboot_mmap->entry_size);
    }

    return ret;
}

const struct multiboot_mmap_entry* get_mmap_entry(unsigned int index)
{
    if (unlikely(!multiboot_mmap))
        return NULL;

    size_t offset = multiboot_mmap->entry_size * index;
    const struct multiboot_mmap_entry* ret = 
        (struct multiboot_mmap_entry*)((u8*)multiboot_mmap->entries + offset);

    if ((u8*)ret >= (u8*)multiboot_mmap + multiboot_mmap->tag.size || ret < multiboot_mmap->entries)
        return NULL;

    return ret;
}

void store_mbi_mmap(const struct multiboot_tag_mmap* mmap_tag)
{
    multiboot_mmap = mmap_tag;
}
