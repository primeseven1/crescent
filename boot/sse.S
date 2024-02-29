.code32

#include <crescent/asm/paging.h>

.section .data
fcw: .word 0x037F
mxcsr: .long 0x01F80

.section .early.text, "ax"
.globl enable_sse_instructions

enable_sse_instructions:
    fninit
    fldcw V2P(fcw)

    movl %cr0, %eax
    movl %cr4, %edx
    andl $-5, %eax
    orb $6, %dh
    orl $2, %eax
    movl %eax, %cr0
    movl %edx, %cr4

    ldmxcsr V2P(mxcsr)

    ret 
