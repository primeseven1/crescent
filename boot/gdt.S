.code32

#include <crescent/asm/segment.h>

.section .rodata
/* 
 * Since this is stored in read only memory, 
 * we need to set the accessed bit in the access byte in the segment descriptor
 * otherwise it's gonna page fault since the CPU sets that bit if it isn't set already
 */
.align 8
gdt_start:
    .quad 0
kcs:
    .word 0xFFFF, 0x0000
    .byte 0x00, 0x9B, 0xC0, 0x00
kds:
    .word 0xFFFF, 0x0000
    .byte 0x00, 0x93, 0xC0, 0x00
ucs:
    .word 0xFFFF, 0x0000
    .byte 0x00, 0xFB, 0xC0, 0x00
uds:
    .word 0xFFFF, 0x0000
    .byte 0x00, 0xF3, 0xC0, 0x00
gdt_end:

.section .text
.globl do_gdt_init

do_gdt_init:
    pushl %ebp
    movl %esp, %ebp

    subl $16, %esp

    movw $(gdt_end - gdt_start - 1), -6(%ebp)
    movl $gdt_start, -4(%ebp)

    lgdt -6(%ebp)
    
    pushl $GDT_SEGMENT_KERNEL_CODE
    leal .reload_cs, %eax
    pushl %eax
    lret
.reload_cs:
    movw $GDT_SEGMENT_KERNEL_DATA, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    leave
    ret
