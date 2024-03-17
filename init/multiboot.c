#include <crescent/kernel.h>
#include <crescent/asm/multiboot2.h>
#include <crescent/asm/paging.h>
#include <crescent/panic.h>

static struct multiboot_tag_locations locations;

static const struct multiboot_info* map_mbi(const struct multiboot_info* mbi_paddr)
{
    __attribute__((aligned(4096)))
    static pte_t multiboot_page_table[512];

    int offset = (uintptr_t)mbi_paddr - (uintptr_t)PAGE_ALIGN(mbi_paddr);
    struct multiboot_info* mbi_vaddr = (struct multiboot_info*)(KERNEL_VMA_OFFSET + offset);

    /* Should not error, since no page table should be mapped at the address we want */
    map_page_table(mbi_vaddr, V2P(multiboot_page_table), 0);
    map_page(mbi_vaddr, mbi_paddr, PT_PRESENT);

    /* Multiboot info is too big, multiboot info is usually 4KiB in size or so */
    if (unlikely(mbi_vaddr->total_size >= (u32)0x200000 - offset))
        panic("Multiboot information too big!\n");

    size_t num_pages = (mbi_vaddr->total_size + offset) / 4096;
    /* Should not error since we already made sure the multiboot info isn't too big */
    map_pages((u8*)mbi_vaddr + PAGE_SIZE, (u8*)mbi_paddr + PAGE_SIZE, PT_PRESENT, num_pages);

    return mbi_vaddr;
}

const struct multiboot_tag_locations* get_mbi_tag_locations(const struct multiboot_info* mbi_paddr)
{
    const struct multiboot_info* mbi = map_mbi(mbi_paddr);

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


    return &locations;
}
