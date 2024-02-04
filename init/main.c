#include <crescent/asm/multiboot2.h>
#include <crescent/asm/paging.h>
#include "init.h"

_Noreturn void kernel_main(const struct multiboot_info* mb_info)
{ 
    struct multiboot_tag_locations locations = parse_multiboot_info(mb_info);
    
    while (1)
        asm volatile("hlt");
}

