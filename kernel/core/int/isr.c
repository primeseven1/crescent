#include <crescent/common.h>
#include <crescent/core/panic.h>
#include <crescent/core/printk.h>
#include <crescent/core/int/isr.h>
#include <crescent/lib/string.h>
#include <crescent/asm/flags.h>
#include <crescent/asm/errno.h>

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
    if (isr_handlers[ctx->int_num]) {
        isr_handlers[ctx->int_num](ctx);
        return;
    }

    if (ctx->int_num < 32)
        panic("A CPU execption occurred (%lu, %s), but no handler is available to handle it", 
                ctx->int_num, exception_messages[ctx->int_num]);

    printk("A CPU interrupt occured (%lu), but there is no handler for it", ctx->int_num);
}

void isr_init(void) {
    memset(isr_handlers, 0, sizeof(isr_handlers));
}
