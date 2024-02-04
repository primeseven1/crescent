#include <crescent/asm/paging.h>
#include <crescent/asm/multiboot2.h>
#include "init.h"

static const struct multiboot_info* map_multiboot_info(const struct multiboot_info* mb_info)
{
    uintptr_t mb_info_aligned = (uintptr_t)mb_info / 4096 * 4096;
    const void* mb_info_vaddr = (void*)0xC0100000;
    int err = map_page(mb_info_vaddr, (void*)mb_info_aligned, PT_PRESENT);
    /* Terrible error handling, but no panic function yet, so doing this for now */
    if (err) {
        while (1)
            asm volatile("hlt");
    }

    unsigned long page_offset = (uintptr_t)mb_info - mb_info_aligned;
    mb_info = (struct multiboot_info*)((uintptr_t)mb_info - mb_info_aligned + (uintptr_t)mb_info_vaddr);

    unsigned int num_pages = mb_info->total_size / PAGE_SIZE + 1;

    if (mb_info->total_size >= PAGE_SIZE - page_offset)
        num_pages++;

    err = map_pages((u8*)mb_info_vaddr + PAGE_SIZE, (void*)mb_info_aligned, PT_PRESENT, num_pages);
    /* Here we go again with the terrible error handling */
    if (err) {
        while (1)
            asm volatile("hlt");
    }

    return mb_info;
}

struct multiboot_tag_locations parse_multiboot_info(const struct multiboot_info* mb_info)
{
    mb_info = map_multiboot_info(mb_info);

    struct multiboot_tag_locations locations;

    const struct multiboot_tag* tag = mb_info->tags;
    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        switch(tag->type) {
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
            locations.framebuffer_location = (struct multiboot_tag_framebuffer*)tag;
            break;
        case MULTIBOOT_TAG_TYPE_MMAP:
            locations.mmap_location = (struct multiboot_tag_mmap*)tag;
            break;
        }

        tag = (struct multiboot_tag*)((u8*)tag + ((tag->size + 7) & ~7));
    }

    return locations;
}
