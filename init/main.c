#include <crescent/asm/multiboot2.h>
#include <crescent/asm/paging.h>
#include <crescent/conversions.h>
#include <crescent/kprint.h>
#include <crescent/panic.h>
#include <crescent/mmap.h>

void do_bootstrap_processor_init(void);

static void print_memory(void)
{
    size_t total_mem = get_total_mmap_free_memory() / 0x100000;
    char total_bytes_str[22];
    ulltostr(total_mem, total_bytes_str, 10, sizeof(total_bytes_str));

    DBG_E9_KPRINT("Total Memory: %lu MiB\n", total_mem);

    const char* msg = "Total memory: ";
    const char* mib = " MiB";

    u16* vid_mem = (u16*)(KERNEL_VMA_OFFSET + 0x350000);
    map_page(vid_mem, (void*)0xB8000, PT_PRESENT | PT_READ_WRITE);

    size_t msg_len = __builtin_strlen(msg);
    size_t i;
    for (i = 0; i < msg_len; i++)
        vid_mem[i] = msg[i] | 0x0E << 8;
    for (size_t j = 0; j < __builtin_strlen(total_bytes_str); i++, j++)
        vid_mem[i] = total_bytes_str[j] | 0x0E << 8;
    for (size_t j = 0; j < __builtin_strlen(mib); i++, j++)
        vid_mem[i] = mib[j] | 0x0E << 8;
}

_Noreturn asmlinkage void kernel_main(const struct multiboot_info* mbi_paddr)
{
    const struct multiboot_tag_locations* locations = get_mbi_tag_locations(mbi_paddr);

    store_mbi_mmap(locations->mmap);

    do_bootstrap_processor_init();

    print_memory();

    while (1)
        asm volatile("hlt");
}
