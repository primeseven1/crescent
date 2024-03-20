.code64

#include <crescent/asm/segment.h>

.section .text
.globl _reload_segment_regs

_reload_segment_regs:
    pushq $GDT_SEGMENT_KERNEL_CODE
    movabsq $.reload_cs, %rax
    pushq %rax
    lretq

.reload_cs:
    movw $GDT_SEGMENT_KERNEL_DATA, %ax

    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    ret
