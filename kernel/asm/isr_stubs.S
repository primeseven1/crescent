.code64

#define isr_no_err(n) \
    .globl _isr_##n; \
    _isr_##n: \
        pushq $0; \
        pushq $n; \
        call isr_common; \
        addq $16, %rsp; \
        iretq;

#define isr_err(n) \
    .globl _isr_##n; \
    _isr_##n: \
        pushq $n; \
        call isr_common; \
        addq $8, %rsp; \
        iretq;

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

    cld

    /* Interrupt frame */
    movq %rsp, %rdi
    addq $128, %rdi

    /* Pointer to the saved general purpose registers */
    movq %rsp, %rsi

    /* 
     * Save SSE/MMX/x87 context, add 8 to the subtraction to align RSP by 16 
     * there is no GP exception when executing fxsave, 
     * this does assume the stack itself is aligned
     */
    subq $(512 + 8), %rsp
    fxsave (%rsp)

    call _do_isr

    /* Restore SSE/MMX/x87 context */
    fxrstor (%rsp)
    addq $(512 + 8), %rsp
   
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

isr_no_err(0)
isr_no_err(1)
isr_no_err(2)
isr_no_err(3)
isr_no_err(4)
isr_no_err(5)
isr_no_err(6)
isr_no_err(7)
isr_err(8)
isr_no_err(9)
isr_err(10)
isr_err(11)
isr_err(12)
isr_err(13)
isr_err(14)
isr_no_err(15)
isr_no_err(16)
isr_err(17)
isr_no_err(18)
isr_no_err(19)
isr_no_err(20)
isr_err(21)
isr_no_err(22)
isr_no_err(23)
isr_no_err(24)
isr_no_err(25)
isr_no_err(26)
isr_no_err(27)
isr_no_err(28)
isr_err(29)
isr_err(30)
isr_no_err(31)

