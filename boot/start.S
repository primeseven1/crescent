.code32

#include <crescent/asm/paging.h>

.section .bss
.align 16
stack_bottom:
    .skip 1024 * 8
stack_top:

.section .early.text, "ax"
.globl _start

_start:
    cli

    /* not using virtual memory, so load the physical address */
    leal V2P(stack_top), %esp
    
    call do_early_checks
    call map_higher_half

    leal start_kernel, %eax
    jmp *%eax

.section .text
start_kernel:
    /* using virtual memory now, so reload esp */
    leal stack_top, %esp

    /* these 3 instructions unmap the indentity mapped page table */
    movl %cr3, %eax
    movl $0, (%eax)
    movl %eax, %cr3

    call do_gdt_init

    /* kernel_main never returns, multiboot info is in ebx */
    push %ebx
    call kernel_main
    ud2
