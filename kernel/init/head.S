#include <crescent/asm/cpuid.h>
#include <crescent/asm/ctl.h>

.code64
.text

.globl _start
.type _start, @function
_start:
    cli
    cld

    call mmx_sse_sanity_check
    testb %al, %al
    jz .halt
    call enable_sse

    call kernel_main
    ud2
.halt:
    hlt
    jmp .halt
.size _start, . - _start

.type mmx_sse_sanity_check, @function
mmx_sse_sanity_check:
    pushq %rbx

    movl $CPUID_LEAF_FEATURE_BITS, %eax
    xorl %ecx, %ecx
    cpuid

    testl $(1 << 23), %edx
    jz .failed
    testl $(1 << 25), %edx
    jz .failed
    testl $(1 << 26), %edx
    jz .failed

    movb $1, %al
    jmp .ret
.failed:
    movb $0, %al
.ret:
    popq %rbx
    ret
.size mmx_sse_sanity_check, . - mmx_sse_sanity_check

.type enable_sse, @function
enable_sse:
    subq $16, %rsp

    fninit
    movw $0x037F, (%rsp)
    fldcw (%rsp)

    movq %cr0, %rax
    andq $(~CR0_EM), %rax
    orq $CR0_MP, %rax
    movq %rax, %cr0
    movq %cr4, %rax
    orq $(CR4_OSFXSR | CR4_OSXMMEXCEPT), %rax
    movq %rax, %cr4
    movl $0x01F80, (%rsp)
    ldmxcsr (%rsp)

    addq $16, %rsp
    ret
.size enable_sse, . - enable_sse
