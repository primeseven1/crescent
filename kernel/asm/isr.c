#include "crescent/kprint.h"
#include <crescent/kernel.h>
#include <crescent/panic.h>

/* These structs probably don't need to be packed, but do it anway to avoid problems */

struct interrupt_frame {
    u64 int_num; /*  The code pushes this on the stack, not the CPU */
    u64 err_code; /* If the interrupt doesn't push an error code, the code pushes 0 */
    void* rip; /* Return address */
    u64 cs; /* Previous code segment */
    u64 rflags; /* Previous rflags */
    void* rsp; /* Previous stack pointer */
    u64 ss; /* Previous stack segment */
} __attribute__((packed));

struct general_regs {
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;
    void* rbp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
} __attribute__((packed));

static const char* exception_messages[31] = {
    "Divide error",
    "Debug exception",
    "NMI",
    "Breakpoint exception",
    "Overflow exception",
    "Bound brange exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor Segment Overrun",
    "Invalid task state segment",
    "Segment not present",
    "Stack segment fault",
    "General protection fault",
    "Page fault",
    "Reserved exception",
    "x87 floating point exception",
    "Alignment check exception",
    "Machine check exception",
    "SIMD Floating point exception",
    "Virtualization exception",
    "Control protection exception",
    "Reserved exception",
    "Reserved exception",
    "Reserved exception",
    "Reserved exception",
    "Reserved exception",
    "Hypervisor Injection exception",
    "VMM Communication exception",
    "Security exception",
    "Reserved exception"
};

asmlinkage void _do_isr(const struct interrupt_frame* frame, const struct general_regs* gp_regs)
{
    if (frame->int_num < 32)
        panic("%lu %s, err: %lu, rip: %p, cs: 0x%lx, rflags: %lu, rsp: %p, ss: 0x%lx",
            frame->int_num, exception_messages[frame->int_num], frame->err_code, frame->rip,
            frame->cs, frame->rflags, frame->rsp, frame->ss);

    DBG_E9_KPRINT("Recived interrupt %lu regs: %p\n", frame->int_num, gp_regs);
}
