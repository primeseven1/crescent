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
#include <crescent/lib/printf.h>

__attribute__((noinline)) 
static void print_stack_trace(void) {
    void** stack_frame = __builtin_frame_address(0);

    size_t kernel_offset = (uintptr_t)&_lds_kernel_start - KERNEL_MIN_VIRTADDR;
    printk(PL_EMERG "Stack trace (kernel offset: 0x%lx):\n", kernel_offset);
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
                printk(PL_EMERG " %p <%s+0x%lx>\n", ret_addr, name, (size_t)offset);
            else
                printk(PL_EMERG " %p <%s+?>\n", ret_addr, name);
        } else {
            printk(PL_EMERG " %p <?+?>\n", ret_addr);
        }

        stack_frame = stack_frame[0];
    }
}

static void print_processor_state(struct cpu* cpu) {
    u64 rsp, rbp;
    u64 cr0, cr2, cr3, cr4;
    u64 fs, gs, krnlgs;
    u64 cs, ds, es;
    u64 rflags;

    /* 
     * No reason in printing general purpose registers,
     * since those registers are likely already trashed
     */
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

    printk(PL_EMERG "Processor %u state: \n", cpu->processor_id);
    printk(PL_EMERG " RSP: 0x%lx, RBP: 0x%lx\n", rsp, rbp);
    printk(PL_EMERG " CR0: 0x%lx, CR2: 0x%lx, CR3: 0x%lx, CR4: 0x%lx\n", cr0, cr2, cr3, cr4);
    printk(PL_EMERG " FS: 0x%lx, GS: 0x%lx, KRNLGS: 0x%lx\n", fs, gs, krnlgs);
    printk(PL_EMERG " CS: 0x%lx, DS: 0x%lx, ES: 0x%lx\n", cs, ds, es);
    printk(PL_EMERG " RFLAGS: 0x%lx\n", rflags);
}

__attribute__((sysv_abi))
void asm_idt_load(void* entries, size_t size);

_Noreturn void panic(const char* fmt, ...) {
    static char buf[512];
    static spinlock_t lock = SPINLOCK_INITIALIZER;

    /* 
     * Since we are trying to panic, we obviously want no IRQ's happening.
     * Also load the IDT with a limit of 0 so that if an NMI or another fault
     * happens, the system just triple faults.
     */
    local_irq_disable();
    asm_idt_load(NULL, 0);

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
    printk(PL_EMERG "--- [ Kernel panic - %s ] ---\n", buf);
    print_processor_state(cpu);
    print_stack_trace();
    printk(PL_EMERG "--- [ End kernel panic - %s ] ---\n", buf);

    va_end(va);
    while (1)
        __asm__ volatile("hlt");
}
