#include <crescent/asm/multiboot2.h>
#include <crescent/asm/paging.h>

struct multiboot_tag_locations* get_mbi_tag_locations(const struct multiboot_info* mbi_paddr);

_Noreturn void kernel_main(const struct multiboot_info* mbi_paddr)
{
    const struct multiboot_tag_locations* locations = get_mbi_tag_locations(mbi_paddr);

    /* Just to print hello world, nothing else, this will be removed later */
    u16* vid_mem = (u16*)(KERNEL_VMA_OFFSET + 0x350000);
    map_page(vid_mem, (void*)0xB8000, PT_PRESENT | PT_READ_WRITE);

    const char* string = "Hello world!";
    for (int i = 0; i < 80; i++)
        vid_mem[i] = 0;

    for (size_t i = 0; i < __builtin_strlen(string); i++)
        vid_mem[i] = string[i] | 0x0E << 8;

    while (1)
        asm volatile("hlt");
}
