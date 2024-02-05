#include <crescent/asm/paging.h>
#include <crescent/asm/multiboot2.h>
#include "init.h"

static const struct multiboot_info* map_multiboot_info(const struct multiboot_info* mbi)
{
    /* First get the offset in the page */
    uintptr_t mbi_paddr_aligned = (uintptr_t)ALIGN_TO_PAGE(mbi);
    uintptr_t mbi_page_offset = (uintptr_t)mbi - mbi_paddr_aligned;

    /* Now map 1 page, and then determine if we need to map more pages */
    mbi = (struct multiboot_info*)(0xC0100000 + mbi_page_offset);

    /* This is really bad error handling, it will be changed when there is a better way to do it */
    int err = map_page(ALIGN_TO_PAGE(mbi), (void*)mbi_paddr_aligned, PT_PRESENT);
    if (err)
        asm volatile("ud2");

    size_t num_pages = (mbi->total_size + mbi_page_offset) / 4096;
    if (num_pages) {
        err = map_pages((void*)((uintptr_t)ALIGN_TO_PAGE(mbi) + 4096), 
            (void*)mbi_paddr_aligned, PT_PRESENT, num_pages);
        if (err)
            asm volatile("ud2");
    }

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
        default:
            break;
        }

        tag = (struct multiboot_tag*)((u8*)tag + ((tag->size + 7) & ~7));
    }

    return locations;
}
