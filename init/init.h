#pragma once

#include <crescent/asm/multiboot2.h>

struct multiboot_tag_locations parse_multiboot_info(const struct multiboot_info* mb_info);
