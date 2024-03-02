#include <crescent/kernel.h>
#include <crescent/mmap.h>

static const struct multiboot_tag_mmap* multiboot_mmap = NULL;

size_t get_total_free_memory(void)
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

void store_mbi_mmap(const struct multiboot_tag_mmap* mmap_tag)
{
    multiboot_mmap = mmap_tag;
}
