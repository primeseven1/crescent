#include <crescent/asm/msr.h>
#include <crescent/asm/segment.h>

.code64

#define swapgs_if_needed \
    cmpq $SEGMENT_KERNEL_CODE, 16(%rsp); \
    je 1f; \
    swapgs; \
    1:

/* 
 * Should only be used for NMI's, Double Faults, and MCE's, 
 * since checking MSR's like this is expensive 
 */
#define swapgs_if_needed_msr \
    pushq %rax; \
    pushq %rcx; \
    pushq %rdx; \
    movl $MSR_GS_BASE, %ecx; \
    rdmsr; \
    shlq $32, %rdx; \
    movl %eax, %eax; \
    orq %rax, %rdx; \
    jne 1f; \
    swapgs; \
    1: \
    popq %rdx; \
    popq %rcx; \
    popq %rdx;

#define isr_no_err(n) \
    .text; \
    .globl _isr_##n; \
    _isr_##n: \
        swapgs_if_needed; \
        pushq $0; \
        pushq $n; \
        call isr_common; \
        addq $16, %rsp; \
        swapgs_if_needed; \
        iretq; \
    .size _isr_##n, . - _isr_##n; \
    .previous;

#define isr_err(n) \
    .text; \
    .globl _isr_##n; \
    _isr_##n: \
        swapgs_if_needed; \
        pushq $n; \
        call isr_common; \
        addq $16, %rsp; \
        swapgs_if_needed; \
        iretq; \
    .size _isr_##n, . - _isr_##n; \
    .previous;

#define isr_no_err_msr(n) \
    .text; \
    .globl _isr_##n; \
    _isr_##n: \
        swapgs_if_needed_msr; \
        pushq $0; \
        pushq $n; \
        call isr_common; \
        addq $16, %rsp; \
        swapgs_if_needed; \
        iretq; \
    .size _isr_##n, . - _isr_##n; \
    .previous;

#define isr_err_msr(n) \
    .text; \
    .globl _isr_##n; \
    _isr_##n: \
        swapgs_if_needed_msr; \
        pushq $n; \
        call isr_common; \
        addq $16, %rsp; \
        swapgs_if_needed; \
        iretq; \
    .size _isr_##n, . - _isr_##n; \
    .previous;

.text
isr_common:
    cld

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

    /* Align the stack for fxsave, use rbx since it's a preserved register in the system-v abi */
    movq %rsp, %rbx
    andq $15, %rbx

    subq %rbx, %rsp
    subq $512, %rsp
    fxsave (%rsp)

    /* Set the mxcsr and fcw registers to what it usually is */
    subq $16, %rsp
    fninit
    movw $0x037F, (%rsp)
    fldcw (%rsp)
    movq $0x01F80, (%rsp)
    ldmxcsr (%rsp)
    addq $16, %rsp
    
    call _do_isr

    fxrstor (%rsp)
    addq $512, %rsp
    addq %rbx, %rsp

    popq %rax /* we don't care about cr2 anymore, so just pop into rax */
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

/* Exceptions */
isr_no_err(0)
isr_no_err(1)
isr_no_err_msr(2)
isr_no_err(3)
isr_no_err(4)
isr_no_err(5)
isr_no_err(6)
isr_no_err(7)
isr_err_msr(8)
isr_no_err(9)
isr_err(10)
isr_err(11)
isr_err(12)
isr_err(13)
isr_err(14)
isr_no_err(15)
isr_no_err(16)
isr_err(17)
isr_no_err_msr(18)
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

/* Not exceptions */
isr_no_err(32)
isr_no_err(33)
isr_no_err(34)
isr_no_err(35)
isr_no_err(36)
isr_no_err(37)
isr_no_err(38)
isr_no_err(39)
isr_no_err(40)
isr_no_err(41)
isr_no_err(42)
isr_no_err(43)
isr_no_err(44)
isr_no_err(45)
isr_no_err(46)
isr_no_err(47)
isr_no_err(48)
isr_no_err(49)
isr_no_err(50)
isr_no_err(51)
isr_no_err(52)
isr_no_err(53)
isr_no_err(54)
isr_no_err(55)
isr_no_err(56)
isr_no_err(57)
isr_no_err(58)
isr_no_err(59)
isr_no_err(60)
isr_no_err(61)
isr_no_err(62)
isr_no_err(63)
isr_no_err(64)
isr_no_err(65)
isr_no_err(66)
isr_no_err(67)
isr_no_err(68)
isr_no_err(69)
isr_no_err(70)
isr_no_err(71)
isr_no_err(72)
isr_no_err(73)
isr_no_err(74)
isr_no_err(75)
isr_no_err(76)
isr_no_err(77)
isr_no_err(78)
isr_no_err(79)
isr_no_err(80)
isr_no_err(81)
isr_no_err(82)
isr_no_err(83)
isr_no_err(84)
isr_no_err(85)
isr_no_err(86)
isr_no_err(87)
isr_no_err(88)
isr_no_err(89)
isr_no_err(90)
isr_no_err(91)
isr_no_err(92)
isr_no_err(93)
isr_no_err(94)
isr_no_err(95)
isr_no_err(96)
isr_no_err(97)
isr_no_err(98)
isr_no_err(99)
isr_no_err(100)
isr_no_err(101)
isr_no_err(102)
isr_no_err(103)
isr_no_err(104)
isr_no_err(105)
isr_no_err(106)
isr_no_err(107)
isr_no_err(108)
isr_no_err(109)
isr_no_err(110)
isr_no_err(111)
isr_no_err(112)
isr_no_err(113)
isr_no_err(114)
isr_no_err(115)
isr_no_err(116)
isr_no_err(117)
isr_no_err(118)
isr_no_err(119)
isr_no_err(120)
isr_no_err(121)
isr_no_err(122)
isr_no_err(123)
isr_no_err(124)
isr_no_err(125)
isr_no_err(126)
isr_no_err(127)
isr_no_err(128)
isr_no_err(129)
isr_no_err(130)
isr_no_err(131)
isr_no_err(132)
isr_no_err(133)
isr_no_err(134)
isr_no_err(135)
isr_no_err(136)
isr_no_err(137)
isr_no_err(138)
isr_no_err(139)
isr_no_err(140)
isr_no_err(141)
isr_no_err(142)
isr_no_err(143)
isr_no_err(144)
isr_no_err(145)
isr_no_err(146)
isr_no_err(147)
isr_no_err(148)
isr_no_err(149)
isr_no_err(150)
isr_no_err(151)
isr_no_err(152)
isr_no_err(153)
isr_no_err(154)
isr_no_err(155)
isr_no_err(156)
isr_no_err(157)
isr_no_err(158)
isr_no_err(159)
isr_no_err(160)
isr_no_err(161)
isr_no_err(162)
isr_no_err(163)
isr_no_err(164)
isr_no_err(165)
isr_no_err(166)
isr_no_err(167)
isr_no_err(168)
isr_no_err(169)
isr_no_err(170)
isr_no_err(171)
isr_no_err(172)
isr_no_err(173)
isr_no_err(174)
isr_no_err(175)
isr_no_err(176)
isr_no_err(177)
isr_no_err(178)
isr_no_err(179)
isr_no_err(180)
isr_no_err(181)
isr_no_err(182)
isr_no_err(183)
isr_no_err(184)
isr_no_err(185)
isr_no_err(186)
isr_no_err(187)
isr_no_err(188)
isr_no_err(189)
isr_no_err(190)
isr_no_err(191)
isr_no_err(192)
isr_no_err(193)
isr_no_err(194)
isr_no_err(195)
isr_no_err(196)
isr_no_err(197)
isr_no_err(198)
isr_no_err(199)
isr_no_err(200)
isr_no_err(201)
isr_no_err(202)
isr_no_err(203)
isr_no_err(204)
isr_no_err(205)
isr_no_err(206)
isr_no_err(207)
isr_no_err(208)
isr_no_err(209)
isr_no_err(210)
isr_no_err(211)
isr_no_err(212)
isr_no_err(213)
isr_no_err(214)
isr_no_err(215)
isr_no_err(216)
isr_no_err(217)
isr_no_err(218)
isr_no_err(219)
isr_no_err(220)
isr_no_err(221)
isr_no_err(222)
isr_no_err(223)
isr_no_err(224)
isr_no_err(225)
isr_no_err(226)
isr_no_err(227)
isr_no_err(228)
isr_no_err(229)
isr_no_err(230)
isr_no_err(231)
isr_no_err(232)
isr_no_err(233)
isr_no_err(234)
isr_no_err(235)
isr_no_err(236)
isr_no_err(237)
isr_no_err(238)
isr_no_err(239)
isr_no_err(240)
isr_no_err(241)
isr_no_err(242)
isr_no_err(243)
isr_no_err(244)
isr_no_err(245)
isr_no_err(246)
isr_no_err(247)
isr_no_err(248)
isr_no_err(249)
isr_no_err(250)
isr_no_err(251)
isr_no_err(252)
isr_no_err(253)
isr_no_err(254)
isr_no_err(255)
