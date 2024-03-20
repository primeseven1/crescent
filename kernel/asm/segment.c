#include <crescent/asm/segment.h>

enum gdt_access_flags {
    GDT_ACCESS_ACCESSED = (1 << 0),
    GDT_ACCESS_READ_WRITE = (1 << 1),
    GDT_ACCESS_CODE_CONFORMING = (1 << 2),
    GDT_ACCESS_EXECUTABLE = (1 << 3),
    GDT_ACCESS_CODE_DATA = (1 << 4),
    GDT_ACCESS_DPL0 = 0,
    GDT_ACCESS_DPL1 = (1 << 5),
    GDT_ACCESS_DPL2 = (2 << 5),
    GDT_ACCESS_DPL3 = (3 << 5),
    GDT_ACCESS_PRESENT = (1 << 7),

    /* When GDT_ACCESS_CODE_DATA_SEGMENT isn't set */
    GDT_ACCESS_TSS_AVAILABLE = (0x09 << 0),
    GDT_ACCESS_TSS_BUSY = (0x0B << 0)
};

enum gdt_granularity_flags {
    GDT_FLAG_64_BIT = (1 << 5),
    GDT_FLAG_32_BIT = (1 << 6),
    GDT_FLAG_16_BIT = 0,

    GDT_FLAG_GRANULARITY_1B = 0,
    GDT_FLAG_GRANULARITY_4K = (1 << 7)
};

static void write_gdt_entry(struct segment_descriptor_common* desc,
    u32 limit, u32 base, u8 access, u8 granularity)
{
    desc->limit_low = limit & 0xFFFF;
    desc->base_low = base & 0xFFFF;
    desc->base_middle = (base >> 16) & 0xFF;
    desc->access = access;
    desc->granularity = granularity;
    desc->base_high = (base >> 24) & 0xFF;
}

static void write_tss_entry(struct segment_descriptor_tss* desc, struct tss_struct* tss_ptr)
{
    write_gdt_entry(&desc->common, sizeof(struct tss_struct), (u64)tss_ptr & 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_DPL0 | GDT_ACCESS_TSS_AVAILABLE, 
        GDT_FLAG_GRANULARITY_1B);

    desc->base_top = (u64)tss_ptr >> 32;
    desc->reserved = 0;
}

void do_gdt_init(struct gdt_struct* gdt_ptr, struct tss_struct* tss_ptr)
{
    write_gdt_entry(&gdt_ptr->segments[0], 0, 0, 0, 0);
    write_gdt_entry(&gdt_ptr->segments[1], 0xFFFFF, 0, 
        GDT_ACCESS_PRESENT | GDT_ACCESS_DPL0 | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_CODE_DATA,
        GDT_FLAG_64_BIT | GDT_FLAG_GRANULARITY_4K);
    write_gdt_entry(&gdt_ptr->segments[2], 0xFFFFF, 0, 
        GDT_ACCESS_PRESENT | GDT_ACCESS_DPL0 | GDT_ACCESS_READ_WRITE | GDT_ACCESS_CODE_DATA,
        GDT_FLAG_32_BIT | GDT_FLAG_GRANULARITY_4K);
    write_gdt_entry(&gdt_ptr->segments[3], 0xFFFFF, 0, 
        GDT_ACCESS_PRESENT | GDT_ACCESS_DPL3 | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_CODE_DATA,
        GDT_FLAG_64_BIT | GDT_FLAG_GRANULARITY_4K);
    write_gdt_entry(&gdt_ptr->segments[4], 0xFFFFF, 0, 
        GDT_ACCESS_PRESENT | GDT_ACCESS_DPL0 | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_CODE_DATA,
        GDT_FLAG_32_BIT | GDT_FLAG_GRANULARITY_4K);
    write_gdt_entry(&gdt_ptr->segments[5], 0xFFFFF, 0, 
        GDT_ACCESS_PRESENT | GDT_ACCESS_DPL0 | GDT_ACCESS_READ_WRITE | GDT_ACCESS_CODE_DATA,
        GDT_FLAG_32_BIT | GDT_FLAG_GRANULARITY_4K);
    write_tss_entry(&gdt_ptr->tss, tss_ptr);
}

void do_gdt_load(const struct gdt_struct* gdt_ptr)
{
    struct {
        u16 limit;
        const struct gdt_struct* ptr;
    } __attribute__((packed)) gdtr = {
        .limit = sizeof(struct gdt_struct) - 1,
        .ptr = gdt_ptr
    };

    asm volatile("lgdt %0" : : "m"(gdtr));
    _reload_segment_regs();
    asm volatile("ltr %0" : : "r"((u16)GDT_SEGMENT_TSS)); 
}
