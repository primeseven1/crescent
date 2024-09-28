#include <crescent/common.h>
#include <crescent/core/cpu.h>
#include <crescent/core/panic.h>
#include <crescent/core/printk.h>
#include <crescent/asm/segment.h>
#include <crescent/lib/string.h>
#include <crescent/mm/zone.h>
#include <crescent/mm/hhdm.h>

/*
 * Segment 0: NULL
 * Segment 1: Kernel code, 64 bit
 * Segment 2: Kernel data, 32 bit (irrelevant)
 * Segment 3: User code, 64 bit
 * Segment 4: User code, 32 bit
 * Segment 5: User data, 32 bit (irrelevant)
 */
static const struct segment_descriptor base[6] = {
    { 
        .limit_low = 0, .base_low = 0, .base_middle = 0, 
        .access = 0, .flags = 0, .base_high = 0 
    },
    {
        .limit_low = 0xFFFF, .base_low = 0, .base_middle = 0,
        .access = 0x9B, .flags = 0xAF, .base_high = 0
    },
    {
        .limit_low = 0xFFFF, .base_low = 0, .base_middle = 0,
        .access = 0x93, .flags = 0xCF, .base_high = 0
    },
    {
        .limit_low = 0xFFFF, .base_low = 0, .base_middle = 0,
        .access = 0xFA, .flags = 0xAF, .base_high = 0
    },
    {
        .limit_low = 0xFFFF, .base_low = 0, .base_middle = 0,
        .access = 0xFA, .flags = 0xCF, .base_high = 0
    },
    {
        .limit_low = 0xFFFF, .base_low = 0, .base_middle = 0,
        .access = 0xF3, .flags = 0xCF, .base_high = 0
    }
};

/* Load the GDT with the assembly instruction */
static inline void gdt_load(struct kernel_segments* segments) {
    struct {
        u16 limit;
        struct kernel_segments* segments;
    } __attribute__((packed)) gdtr = {
        .limit = sizeof(*segments) - 1,
        .segments = segments
    };

    __asm__ volatile("lgdt %0" : : "m"(gdtr) : "memory");
}

/* Load the TSS with the assembly instruction */
static inline void tss_load(void) {
    __asm__ volatile("ltr %0" : : "r"((u16)SEGMENT_TASK_STATE) : "memory");
}

static void reload_segment_regs(void) {
    __asm__ volatile("swapgs\n\t"
            "movw $0, %%ax\n\t"
            "movw %%ax, %%gs\n\t"
            "movw %%ax, %%fs\n\t"
            "swapgs\n\t"
            "movw $0x10, %%ax\n\t"
            "movw %%ax, %%ds\n\t"
            "movw %%ax, %%es\n\t"
            "movw %%ax, %%ss\n\t"
            "pushq $0x8\n\t"
            "leaq .reload(%%rip), %%rax\n\t"
            "pushq %%rax\n\t"
            "lretq\n\t"
            ".reload:"
            :
            :
            : "rax", "memory");
}

void segments_init(void) {
    int errno;
    struct kernel_segments* segments = kmmap(NULL, sizeof(*segments), KMMAP_ALLOC,
            MMU_READ | MMU_WRITE, GFP_VM_KERNEL | GFP_PM_ZONE_NORMAL, NULL, &errno);
    if (!segments)
        panic("Failed to allocate GDT, errno: %i", errno);

    /* First copy all of the base segments that are the same for all processors */
    memcpy(segments->segments, base, sizeof(segments->segments));

    /* Allocate TSS + io permission bitmap */
    size_t tss_size = sizeof(struct tss_descriptor) + (65536 / 8);
    struct tss_descriptor* tss = kmmap(NULL, tss_size,
            KMMAP_ALLOC, MMU_READ | MMU_WRITE, GFP_VM_KERNEL | GFP_PM_ZONE_NORMAL, 
            NULL, &errno);
    if (!tss)
        panic("Failed to allocate TSS! errno: %i", errno);

    /* 
     * Memset to 1 for IO permission bitmap, so no IO operations 
     * can happen at user level. The other fields will be filled in if
     * no errors happen
     */
    memset(tss, 1, tss_size);

    size_t ist_stack_size = 0x4000;
    int errno1, errno2, errno3;
    void* rsp0 = kmmap(NULL, ist_stack_size, KMMAP_ALLOC, 
            MMU_READ | MMU_WRITE, GFP_VM_KERNEL | GFP_PM_ZONE_NORMAL, 
            NULL, &errno);
    void* ist1 = kmmap(NULL, ist_stack_size, KMMAP_ALLOC, 
            MMU_READ | MMU_WRITE, GFP_VM_KERNEL | GFP_PM_ZONE_NORMAL, 
            NULL, &errno1); 
    void* ist2 = kmmap(NULL, ist_stack_size, KMMAP_ALLOC, 
            MMU_READ | MMU_WRITE, GFP_VM_KERNEL | GFP_PM_ZONE_NORMAL, 
            NULL, &errno2);
    void* ist3 = kmmap(NULL, ist_stack_size, KMMAP_ALLOC, 
            MMU_READ | MMU_WRITE, GFP_VM_KERNEL | GFP_PM_ZONE_NORMAL, 
            NULL, &errno3);
    if (!rsp0 || !ist1 || !ist2 || !ist3)
        panic("Failed to allocate TSS stacks, errno: rsp0: %i ist1: %i ist2: %i ist3: %i", 
                errno, errno1, errno2, errno3);

    /* Now get the virtual addresses of the stacks, and add the size since the stack grows down */
    tss->rsp[0] = (u8*)rsp0 + ist_stack_size;
    tss->ist[0] = (u8*)ist1 + ist_stack_size;
    tss->ist[1] = (u8*)ist2 + ist_stack_size;
    tss->ist[2] = (u8*)ist3 + ist_stack_size;
    tss->iopb = sizeof(*tss);

    /* Now set up the TSS descriptor */
    segments->tss.desc.limit_low = sizeof(*tss);
    segments->tss.desc.base_low = (uintptr_t)tss & 0xFFFF;
    segments->tss.desc.base_middle = ((uintptr_t)tss >> 16) & 0xFF;
    segments->tss.desc.access = 0x89;
    segments->tss.desc.flags = 0;
    segments->tss.desc.base_high = ((uintptr_t)tss >> 24) & 0xFF;
    segments->tss.base_top = (uintptr_t)tss >> 32;
    segments->tss.reserved = 0;

    gdt_load(segments);
    reload_segment_regs();
    tss_load();

    printk("GDT Loaded on processor %u\n", _cpu()->processor_id);
}
