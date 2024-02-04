#include <crescent/asm/multiboot2.h>
#include <crescent/asm/paging.h>
#include "init.h"

_Noreturn void kernel_main(const struct multiboot_info* mbi)
{
    struct multiboot_tag_locations locations = get_multiboot_tag_locations(mbi);

    while (1)
        asm volatile("hlt");
}
