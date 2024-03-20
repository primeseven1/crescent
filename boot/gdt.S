.code32

#include <crescent/asm/paging.h>

.section .rodata
.align 8
gdt_start:
    .quad 0
kcs:
    .word 0xFFFF, 0x0000
    .byte 0x00, 0x9B, 0xA0, 0x00
kds:
    .word 0xFFFF, 0x0000
    .byte 0x00, 0x93, 0xC0, 0x00
gdt_end:

gdtr:
    .word gdt_end - gdt_start - 1
    .quad gdt_start

.section .early.text, "ax"
.globl do_boot_gdt_init

do_boot_gdt_init:
    lgdt V2P(gdtr)
    ret
