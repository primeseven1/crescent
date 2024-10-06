#include <crescent/common.h>
#include <crescent/core/cpu.h>
#include <crescent/core/panic.h>
#include <crescent/core/printk.h>
#include <crescent/core/int/isr.h>
#include <crescent/lib/string.h>
#include <crescent/asm/flags.h>
#include <crescent/asm/errno.h>
#include "i8259.h"

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

static void (*isr_handlers[256])(const struct ctx* ctx);

void _do_isr(const struct ctx* ctx);
void _do_isr(const struct ctx* ctx) {
    if (ctx->int_num < 32) {
        if (isr_handlers[ctx->int_num]) {
            isr_handlers[ctx->int_num](ctx);
            return;
        }

        panic("A CPU execption occurred (%lu, %s), but no handler is available to handle it", 
                ctx->int_num, exception_messages[ctx->int_num]);
    }

    if (ctx->int_num < 48) {
        u8 irq = ctx->int_num - 32;
        if (i8259_is_irq_spurious(irq)) {
            i8259_handle_spurious_irq(irq);
            printk(PL_WARN "core: int: Spurious IRQ from the i8259 PIC on CPU %u\n", _cpu()->processor_id);
            return;
        }
        
        if (isr_handlers[ctx->int_num])
            isr_handlers[ctx->int_num](ctx);
        else
            printk(PL_CRIT "core: int: Interrupt %lu (irq %u) recived on CPU %u, but there is no handler to handle it\n",
                    ctx->int_num, irq, _cpu()->processor_id);

        i8259_send_eoi(irq);
        return;
    }

    printk(PL_CRIT "core: int: A CPU interrupt occured (%lu) on cpu %u, but there is no handler for it\n",
            ctx->int_num, _cpu()->processor_id);
}

void isr_init(void) {
    memset(isr_handlers, 0, sizeof(isr_handlers));
    i8259_init();
    printk("core: int: Initialized ISR's\n");
}
