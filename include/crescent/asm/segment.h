#pragma once

#define SEGMENT_KERNEL_CODE 0x08
#define SEGMENT_KERNEL_DATA 0x10
#define SEGMENT_USER_CODE 0x18
#define SEGMENT_USER_CODE_32 0x20
#define SEGMENT_USER_DATA 0x28
#define SEGMENT_TASK_STATE 0x30

#ifndef _ASSEMBLY_

#include <crescent/types.h>

struct segment_descriptor {
    u16 limit_low;
    u16 base_low;
    u8 base_middle;
    u8 access;
    u8 flags;
    u8 base_high;
} __attribute__((packed));

struct tss_segment_descriptor {
    struct segment_descriptor desc;
    u32 base_top;
    u32 reserved;
} __attribute__((packed));

struct tss_descriptor {
    u32 reserved0;
    void* rsp[3];
    u64 reserved1;
    void* ist[7];
    u32 reserved2;
    u32 reserved3;
    u16 reserved4;
    u16 iopb;
} __attribute__((packed));

struct kernel_segments {
    struct segment_descriptor segments[6];
    struct tss_segment_descriptor tss;
} __attribute__((packed));

/**
 * @brief Initialize the global descriptor table
 */
void segments_init(void);

#endif /* _ASSEMBLY_ */
