#pragma once

#include <crescent/kernel.h>

enum idt_exception_nums {
    IDT_EXCEPTION_DIVISION_ERROR = 0,
    IDT_EXCEPTION_DEBUG,
    IDT_EXCEPTION_NMI,
    IDT_EXCEPTION_BREAKPOINT,
    IDT_EXCEPTION_OVERFLOW,
    IDT_EXCEPTION_BOUND_RANGE,
    IDT_EXCEPTION_INVALID_OPCODE,
    IDT_EXCEPTION_DEVICE_NOT_AVAILABLE,
    IDT_EXCEPTION_DOUBLE_FAULT,
    IDT_EXCEPTION_COPROCESSOR_SEGMENT_OVERRUN,
    IDT_EXCEPTION_INVALID_TSS,
    IDT_EXCEPTION_SEGMENT_NOT_PRESENT,
    IDT_EXCEPTION_STACK_SEGMENT,
    IDT_EXCEPTION_GENERAL_PROTECTION,
    IDT_EXCEPTION_PAGE_FAULT,
    IDT_EXCEPTION_X87_FLOATING_POINT = 16,
    IDT_EXCEPTION_ALIGNMENT_CHECK,
    IDT_EXCEPTION_MACHINE_CHECK,
    IDT_EXCEPTION_SIMD_FLOATING_POINT,
    IDT_EXCEPTION_VIRTUALIZATION,
    IDT_EXCEPTION_CONTROL_PROTECTION,
    IDT_EXCEPTION_HYPERVISOR_INJECTION = 28,
    IDT_EXCEPTION_VMM_COMMUNICATION,
    IDT_EXCEPTION_SECURITY
};

/* Never ever call these functions! */
asmlinkage void _isr_0(void);
asmlinkage void _isr_1(void);
asmlinkage void _isr_2(void);
asmlinkage void _isr_3(void);
asmlinkage void _isr_4(void);
asmlinkage void _isr_5(void);
asmlinkage void _isr_6(void);
asmlinkage void _isr_7(void);
asmlinkage void _isr_8(void);
asmlinkage void _isr_9(void);
asmlinkage void _isr_10(void);
asmlinkage void _isr_11(void);
asmlinkage void _isr_12(void);
asmlinkage void _isr_13(void);
asmlinkage void _isr_14(void);
asmlinkage void _isr_15(void);
asmlinkage void _isr_16(void);
asmlinkage void _isr_17(void);
asmlinkage void _isr_18(void);
asmlinkage void _isr_19(void);
asmlinkage void _isr_20(void);
asmlinkage void _isr_21(void);
asmlinkage void _isr_22(void);
asmlinkage void _isr_23(void);
asmlinkage void _isr_24(void);
asmlinkage void _isr_25(void);
asmlinkage void _isr_26(void);
asmlinkage void _isr_27(void);
asmlinkage void _isr_28(void);
asmlinkage void _isr_29(void);
asmlinkage void _isr_30(void);
asmlinkage void _isr_31(void);
