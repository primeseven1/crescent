.code32

#include <crescent/asm/paging.h>
#include <crescent/asm/segment.h>

.section .bss
.align 16
stack_bottom:
    .skip 1024 * 8
stack_top:

.section .early.text, "ax"
.globl _start

_start:
    cli

    leal V2P(stack_top), %esp
    
    pushl %ebx

    call do_early_checks
    call enable_sse_instructions
    call map_higher_half
    call enable_long_mode
    call do_boot_gdt_init

    popl %ebx

    ljmp $GDT_SEGMENT_KERNEL_CODE, $start64

.code64
start64:
    movw $GDT_SEGMENT_KERNEL_DATA, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movabsq $start_kernel, %rax
    jmp *%rax

.section .text
start_kernel:
    movabsq $stack_top, %rsp

    /* Unmap indentity mapping + flush TLB */
    movq %cr3, %rax
    movq $0, (%rax)
    movq %rax, %cr3

    xorq %rdi, %rdi
    movl %ebx, %edi
    call kernel_main
    ud2
