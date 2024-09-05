#include <crescent/core/printk.h>
#include <crescent/core/panic.h>
#include <crescent/core/cpu.h>
#include <crescent/core/debug/e9.h>
#include <crescent/core/debug/trace.h>
#include <crescent/core/int/idt.h>
#include <crescent/core/int/isr.h>
#include <crescent/asm/segment.h>
#include <crescent/mm/zone.h>
#include <crescent/mm/mmap.h>
#include <crescent/mm/hhdm.h>

static struct cpu bsp_cpu;

static void bsp_cpu_init(void) {
    unsigned long* vmm_ctx;
    __asm__("movq %%cr3, %0" : "=r"(vmm_ctx));
    vmm_ctx = hhdm_virtual(vmm_ctx);
    bsp_cpu.vmm_ctx = vmm_ctx;
    _cpu_set(&bsp_cpu);
}

_Noreturn void kernel_main(void);
_Noreturn void kernel_main(void) {
    bsp_cpu_init();
#ifdef CONFIG_E9_ENABLE
    printk_set_hook(debug_e9_write_str);
#endif /* CONFIG_E9_ENABLE */
    int err = tracing_init();
    if (err)
        printk("tracing_init() failed with code %i!\n", err);

    memory_zones_init();
    mmap_init();
    segments_init();
    idt_init();
    isr_init();

    printk("Successfully finished execution!\n");
    while (1)
        __asm__ volatile("hlt");
}
