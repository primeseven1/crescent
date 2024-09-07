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
#include <crescent/drivers/video/video.h>

static struct cpu bsp_cpu;

static void bsp_cpu_init(void) {
    unsigned long* vm_ctx;
    __asm__("movq %%cr3, %0" : "=r"(vm_ctx));
    vm_ctx = hhdm_virtual(vm_ctx);
    bsp_cpu.vm_ctx.ctx = vm_ctx;
    bsp_cpu.vm_ctx.lock = SPINLOCK_INITIALIZER;
    _cpu_set(&bsp_cpu);
}

_Noreturn void kernel_main(void);
_Noreturn void kernel_main(void) {
    bsp_cpu_init();
    int err = driver_video_init();
    if (!err)
        printk_set_hook(driver_video_printk);
    err = tracing_init();
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