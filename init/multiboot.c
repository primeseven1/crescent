#include <crescent/asm/paging.h>
#include <crescent/asm/multiboot2.h>
#include "init.h"

static const struct multiboot_info* map_multiboot_info(const struct multiboot_info* mbi)
{
    uintptr_t mbi_aligned = (uintptr_t)mbi / 4096 * 4096;
    const void* mbi_vaddr = (void*)(KERNEL_VMA_OFFSET + 0x100000);
    int err = map_page(mbi_vaddr, (void*)mbi_aligned, PT_PRESENT);
    /* Terrible error handling, but no panic function yet, so doing this for now */
    if (err)
        asm volatile("ud2");

    unsigned long page_offset = (uintptr_t)mbi - mbi_aligned;
    mbi = (struct multiboot_info*)((uintptr_t)mbi - mbi_aligned + (uintptr_t)mbi_vaddr);

    size_t num_pages = mbi->total_size / PAGE_SIZE + 1;

    if (mbi->total_size >= PAGE_SIZE - page_offset)
        num_pages++;

    err = map_pages((u8*)mbi_vaddr + PAGE_SIZE, (void*)mbi_aligned, PT_PRESENT, num_pages);
    /* Here we go again with the terrible error handling */
    if (err)
        asm volatile("ud2");

    return mbi;
}

struct multiboot_tag_locations get_multiboot_tag_locations(const struct multiboot_info* mbi)
{
    mbi = map_multiboot_info(mbi);

    struct multiboot_tag_locations locations;

    const struct multiboot_tag* tag = mbi->tags;
    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
            locations.framebuffer = (struct multiboot_tag_framebuffer*)tag;
            break;
        case MULTIBOOT_TAG_TYPE_MMAP:
            locations.mmap = (struct multiboot_tag_mmap*)tag;
            break;
        case MULTIBOOT_TAG_TYPE_BOOTDEV:
            locations.boot_device = (struct multiboot_tag_bootdev*)tag;
            break;
        case MULTIBOOT_TAG_TYPE_ACPI_NEW:
            locations.acpi.new = true;
            /* fall through */
        case MULTIBOOT_TAG_TYPE_ACPI_OLD:
            locations.acpi.rsdp = (struct multiboot_tag_acpi*)tag;
            break;
        case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
            locations.bootloader_name = (struct multiboot_tag_string*)tag;
            break;
        }

        tag = (struct multiboot_tag*)((u8*)tag + ((tag->size + 7) & ~7));
    }

    return locations;
}
