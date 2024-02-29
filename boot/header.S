#include <crescent/asm/multiboot2.h>

.section .multiboot2.data, "aw"
.align 8
header_start:
    .long MULTIBOOT2_HEADER_MAGIC
    .long MULTIBOOT_ARCHITECTURE_I386
    .long header_end - header_start
    .long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + (header_end - header_start))
.align 8
end_tag:
    .word MULTIBOOT_HEADER_TAG_END
    .word 0
    .long header_end - end_tag
header_end:

