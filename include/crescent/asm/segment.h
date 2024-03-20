#pragma once

#define GDT_SEGMENT_KERNEL_CODE 0x08
#define GDT_SEGMENT_KERNEL_DATA 0x10
#define GDT_SEGMENT_USER_CODE 0x18
#define GDT_SEGMENT_USER_CODE32 0x20
#define GDT_SEGMENT_USER_DATA 0x28
#define GDT_SEGMENT_TSS 0x30

#ifndef __ASSEMBLY_FILE__

#include <crescent/kernel.h>
#include <crescent/types.h>

struct tss_struct {
    u32 reserved0;
    void* rsp0; /* Stack pointer to switch to when switching to ring 0 (kernel mode) */
    void* rsp1; /* Stack pointer to switch to when switching to ring 1 (unused) */
    void* rsp2; /* Stack pointer to switch to when switching to ring 2 (unused) */
    u64 reserved1;
    void* ist[7]; /* Stack pointers for the IST field in the IDT */
    u64 reserved2;
    u16 iopb; /* Port IO permission bitmap */
    u16 reserved3;
} __attribute__((packed));

struct segment_descriptor_common {
    u16 limit_low; /* Low 16 bits of the segment limit */
    u16 base_low; /* Low 16 bits of the base of the segment */
    u8 base_middle; /* Middle 8 bits of the base for the segment */
    u8 access; /* Access byte */
    u8 granularity; /* Controls 64/32/16 bit modes, and how the segment limit is interpreted */
    u8 base_high; /* High 8 bytes of the base */
} __attribute__((packed));

struct segment_descriptor_tss {
    struct segment_descriptor_common common;
    u32 base_top; /* Top 32 bits of the tss pointer */
    u32 reserved;
} __attribute__((packed));

struct gdt_struct {
    struct segment_descriptor_common segments[6];
    struct segment_descriptor_tss tss;
} __attribute__((packed));

/* Internal use, do not use! */
asmlinkage void _reload_segment_regs(void);

/*
 * Initializes a global descriptor table
 *
 * --- Notes ---
 * - Initializes a GDT like like this: NULL segment, Kernel Code, Kernel Data, 
 * User code, User Code (32 bit), User data, TSS
 *
 * - This will not initialize a TSS for you, you'll have to set that up yourself
 *
 * - This will also not call do_gdt_load for you, you also have to do that yourself
 *
 * --- Parameters ---
 * - gdt_ptr: Pointer to the GDT struct
 */
void do_gdt_init(struct gdt_struct* gdt_ptr, struct tss_struct* tss_ptr);

/*
 * Load a GDT
 *
 * --- Notes ---
 * - This will load the task register with the TSS for you
 * - This will also jump to the kernel code segment, and load kernel data segemnt registers
 *
 * --- Parameters ---
 * - gdt_ptr: Pointer to the GDT, should be aligned by 8 bytes, but doesn't have to be
 */
void do_gdt_load(const struct gdt_struct* gdt_ptr);

#endif /* __ASSEMBLY_FILE__ */
