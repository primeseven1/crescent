#include <crescent/common.h>
#include <crescent/core/printk.h>
#include <crescent/core/panic.h>
#include <crescent/core/cpu.h>
#include <crescent/core/debug/e9.h>
#include <crescent/core/debug/trace.h>
#include <crescent/core/int/idt.h>
#include <crescent/core/int/isr.h>
#include <crescent/asm/segment.h>
#include <crescent/asm/cpuid.h>
#include <crescent/mm/zone.h>
#include <crescent/mm/mmap.h>
#include <crescent/mm/hhdm.h>
#include <crescent/mm/vm_zone.h>
#include <crescent/mm/heap.h>
#include <crescent/drivers/video/video.h>

static struct cpu bsp_cpu;

static void bsp_cpu_init(void) {
    unsigned long* vm_ctx;
    __asm__("movq %%cr3, %0" : "=r"(vm_ctx));
    vm_ctx = hhdm_virtual(vm_ctx);
    bsp_cpu.vm_ctx.ctx = vm_ctx;
    bsp_cpu.vm_ctx.lock = SPINLOCK_INITIALIZER;

    struct limine_smp_response* smp_response = g_limine_smp_request.response;
    if (smp_response) {
        for (u64 i = 0; i < smp_response->cpu_count; i++) {
            if (smp_response->cpus[i]->lapic_id != smp_response->bsp_lapic_id) {
                bsp_cpu.processor_id = smp_response->cpus[i]->processor_id;
                break;
            }
        }
    } else {
        bsp_cpu.processor_id = 0;
    }

    _cpu_set(&bsp_cpu);
}

static void check_cpu_features(void) {
    bool fxsr_missing = false;
    bool nx_missing = false;

    u32 eax, ebx, ecx, edx;
    cpuid(CPUID_LEAF_FEATURE_BITS, 0, &eax, &ebx, &ecx, &edx);
    if (!(edx & (1 << 24)))
        fxsr_missing = true;
    cpuid(CPUID_EXT_LEAF_FEATURE_BITS, 0, &eax, &ebx, &ecx, &edx);
    if (!(edx & (1 << 20)))
        nx_missing = true;

    if (fxsr_missing || nx_missing)
        panic("Missing CPU features: (fxsr: %i) (nx: %i)", fxsr_missing, nx_missing);
}

_Noreturn void kernel_main(void);
_Noreturn void kernel_main(void) {
    bsp_cpu_init();
    int err = driver_video_init();
    if (!err)
        printk_set_hook(driver_video_printk);
    err = tracing_init();
    if (err)
        printk(PL_WARN "tracing_init() failed with code %i!\n", err);

    check_cpu_features();
    memory_zones_init();
    mmap_init();
    vm_zones_init();
    segments_init();
    idt_init();
    isr_init();
    heap_init();

    printk(PL_CRIT "Successfully finished execution!\n");
    while (1)
        __asm__ volatile("hlt");
}
