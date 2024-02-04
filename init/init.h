#pragma once

#include <crescent/asm/multiboot2.h>

struct multiboot_tag_locations get_multiboot_tag_locations(const struct multiboot_info* mbi);
