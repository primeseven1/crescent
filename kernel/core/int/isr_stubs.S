#include <crescent/asm/msr.h>
#include <crescent/asm/segment.h>

.code64

.macro swapgs_if_needed_msr_no
    cmpq $SEGMENT_KERNEL_CODE, 24(%rsp) /* swapgs if interrupt happened in user mode */
    je 1f
    swapgs
1:
.endm

/* 
 * The "paranoid" way of checking swapgs. This should only be used for
 * things like NMI's, and MCE's, since something like an NMI could happen
 * in the middle of swapgs_if_needed
 */
.macro swapgs_if_needed_msr_yes
    pushq %rax
    pushq %rcx
    pushq %rdx
    movl $MSR_GS_BASE, %ecx
    rdmsr
    testl %edx, %edx
    js 1f
    swapgs
1:
    popq %rdx
    popq %rcx
    popq %rax
.endm

/*
 * Calling isr_common is more convenient for the way i've implemented
 * this. The symbol name will also show up in a stack trace if a panic
 * happens.
 */

/* n is the interrupt number, and msr determines whether or not to do the MSR way of doing swapgs */

.macro isr_no_err n, msr
.text
.globl _isr_\n
.type _isr_\n, @function
.align 0x10
_isr_\n:
    pushq $0
    pushq $\n
    swapgs_if_needed_msr_\msr
    cld
    call isr_common
    cli /* make sure no interrupts can happen when handling swapgs */
    swapgs_if_needed_msr_no /* nmi and mce will crash the kernel, so this is fine */
    addq $16, %rsp
    iretq
.size _isr_\n, . - _isr_\n
.previous
.endm

.macro isr_err n, msr
.text
.globl _isr_\n
.type _isr_\n, @function
.align 0x10
_isr_\n:
    pushq $\n
    swapgs_if_needed_msr_\msr
    cld
    call isr_common
    cli /* make sure no interrupts can happen when handling swapgs */
    swapgs_if_needed_msr_no /* double fault will crash the kernel, this is fine */
    addq $16, %rsp /* skip the error code */
    iretq
.size _isr_\n, . - _isr_\n
.previous
.endm

.text
.type isr_common, @function
.align 0x10
isr_common:
	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rbp
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %rbx
	pushq %rax
    movq %cr2, %rax
    pushq %rax

    /* Move this into rdi as a parameter for _do_isr */
    movq %rsp, %rdi

    /* 
     * Align the stack since fxsave requires a 16-aligned stack, 
     * use rbx since it's a preserved register in the system-v abi 
     */
    movq %rsp, %rbx
    andq $15, %rbx

    subq %rbx, %rsp
    subq $512, %rsp
    fxsave (%rsp)

    subq $8, %rsp

    /* 
     * Set the mxcsr and fcw registers to what it usually is for the kernel context,
     * it's important to do this only AFTER fxsave happens
     */
    fninit
    movw $0x037F, (%rsp)
    fldcw (%rsp)
    movq $0x01F80, (%rsp)
    ldmxcsr (%rsp)
    call _do_isr

    addq $8, %rsp

    fxrstor (%rsp)
    addq $512, %rsp
    addq %rbx, %rsp

    popq %rax
    movq %rax, %cr2
    popq %rax
	popq %rbx
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi
	popq %rbp
	popq %r8
	popq %r9
	popq %r10
	popq %r11
	popq %r12
	popq %r13
	popq %r14
	popq %r15
    
    ret
.size isr_common, . - isr_common

/* Now just set up all ISR stubs */
.altmacro
.set i, 0
.rept 256
.if ((i >= 10 && i <= 14) || i == 17 || i == 21 || i == 29 || i == 30)
    isr_err %i no
.elseif (i == 2 || i == 18)
    isr_no_err %i yes
.elseif (i == 8)
    isr_err %i yes
.else
    isr_no_err %i no
.endif
    .set i, i+1
.endr
