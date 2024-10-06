#include <crescent/asm/cpuid.h>
#include <crescent/asm/ctl.h>
#include <crescent/asm/segment.h>

.code64
.text

.globl _start
.type _start, @function
_start:
    cli
    cld
    andq $-16, %rsp /* make sure the stack is aligned */
    call kernel_main
    ud2
.size _start, . - _start

.globl asm_gdt_load
.type asm_gdt_load, @function
asm_gdt_load:
    subq $16, %rsp

    /* si contains the size of the GDT, it must be subtracted by 1 */
    testw %si, %si
    jz 1f
    subw $1, %si
1:
    movw %si, (%rsp)
    movq %rdi, 2(%rsp)
    lgdt (%rsp)

    addq $16, %rsp

    swapgs /* The GSBASE register contains the per-cpu data, so swap it */
    movw $0, %ax
    movw %ax, %gs
    movw %ax, %fs
    swapgs

    movw $SEGMENT_KERNEL_DATA, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

    pushq $SEGMENT_KERNEL_CODE
    leaq .reload(%rip), %rax
    pushq %rax
    lretq
.reload:
    movw $SEGMENT_TASK_STATE, %ax
    ltr %ax
    ret
.size asm_gdt_load, . - asm_gdt_load

.globl asm_idt_load
.type asm_idt_load, @function
asm_idt_load:
    subq $16, %rsp

    /* Like with the GDT, the size must be subtracted by 1 */
    testw %si, %si
    jz 1f
    subw $1, %si
1:
    movw %si, (%rsp)
    movq %rdi, 2(%rsp)
    lidt (%rsp)

    addq $16, %rsp
    ret
.size asm_idt_load, . - asm_idt_load
