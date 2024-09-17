#include <crescent/common.h>
#include <crescent/core/panic.h>
#include <crescent/core/cpu.h>
#include <crescent/core/locking.h>
#include <crescent/core/printk.h>
#include <crescent/core/ld_syms.h>
#include <crescent/core/debug/trace.h>
#include <crescent/core/int/irq.h>
#include <crescent/asm/flags.h>
#include <crescent/asm/msr.h>

__attribute__((noinline)) 
static void print_stack_trace(void) {
    void** stack_frame = __builtin_frame_address(0);

    size_t kernel_offset = (uintptr_t)&_lds_kernel_start - KERNEL_MIN_VIRTADDR;
    printk("--- Stack trace (kernel offset: 0x%lx) --- \n", kernel_offset);
    for (int i = 0; i < 10; i++) {
        if (!stack_frame)
            break;
        void* ret_addr = stack_frame[1];
        if (!ret_addr)
            break;

        const char* name = trace_kernel_symbol_name(ret_addr);
        if (name) {
            ssize_t offset = trace_kernel_symbol_offset(ret_addr);
            if (likely(offset != -1))
                printk(" %p <%s+0x%lx>\n", ret_addr, name, (size_t)offset);
            else
                printk(" %p <%s+?>\n", ret_addr, name);
        } else {
            printk(" %p <?+?>\n", ret_addr);
        }

        stack_frame = stack_frame[0];
    }
}

static void print_registers(void) {
    u64 rsp, rbp;
    u64 cr0, cr2, cr3, cr4;
    u64 fs, gs, krnlgs;
    u64 cs, ds, es;
    u64 rflags;

    __asm__("movq %%rsp, %0\n\t"
            "movq %%rbp, %1\n\t"
            : "=rm"(rsp), "=rm"(rbp));
    __asm__("movq %%cr0, %0\n\t"
            "movq %%cr2, %1\n\t"
            "movq %%cr3, %2\n\t"
            "movq %%cr4, %3\n\t"
            : "=r"(cr0), "=r"(cr2), "=r"(cr3), "=r"(cr4));
    fs = rdmsr(MSR_FS_BASE);
    gs = rdmsr(MSR_GS_BASE);
    krnlgs = rdmsr(MSR_KERNEL_GS_BASE);
    __asm__("movq %%cs, %0\n\t"
            "movq %%ds, %1\n\t"
            "movq %%es, %2\n\t"
            : "=r"(cs), "=r"(ds), "=r"(es)); 
    rflags = read_cpu_flags();
    printk("--- Registers ---\n"
            " RSP: 0x%lx, RBP: 0x%lx\n"
            " CR0: 0x%lx, CR2: 0x%lx, CR3: 0x%lx, CR4: 0x%lx\n"
            " FS: 0x%lx, GS: 0x%lx, KRNLGS: 0x%lx\n"
            " CS: 0x%lx, DS: 0x%lx, ES: 0x%lx\n"
            " RFLAGS: 0x%lx\n",
            rsp, rbp, cr0, cr2, cr3, cr4, fs, gs, krnlgs, cs, ds, es, rflags);
}

static inline void idt_load_null(void) {
    struct {
        u16 limit;
        void* idt_ptr;
    } __attribute__((packed)) idtr = {
        .limit = 0,
        .idt_ptr = NULL
    };

    __asm__ volatile("lidt %0" : : "m"(idtr) : "memory");
}

_Noreturn void panic(const char* fmt, ...) {
    static char buf[512];
    static spinlock_t lock = SPINLOCK_INITIALIZER;

    /* 
     * Since we are trying to panic, we obviously want no IRQ's happening.
     * Also load the IDT with a limit of 0 so that if an NMI or another fault
     * happens, the system just triple faults.
     */
    local_irq_disable();
    idt_load_null();

    /* Unlikely that 2 threads will call panic at the same time, but it could happen */
    spinlock_lock(&lock);

    va_list va;
    va_start(va, fmt);

    /* 
     * I had an issue where a deadlock would happen because a page fault 
     * would happen here since the gsbase register wasn't swapped correctly.
     * This will correct that, and it will swapgs after getting the per-cpu data
     * so it shows up in the register dump.
     */
    struct cpu* cpu = (struct cpu*)rdmsr(MSR_GS_BASE);
    if (unlikely(!cpu)) {
        swapgs();
        cpu = _cpu();
        swapgs();
    }

    vsnprintf(buf, sizeof(buf), fmt, va);
    printk("\n[ Kernel panic (processor %u) - %s ]\n", cpu->processor_id, buf);
    print_registers();
    print_stack_trace();
    printk("[ End kernel panic (processor %u) - %s ]\n", cpu->processor_id, buf);

    va_end(va);
    while (1)
        __asm__ volatile("hlt");
}
