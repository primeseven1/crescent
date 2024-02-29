#pragma once

#include <crescent/types.h>

static inline void msr_write(u64 msr_id, u64 val)
{
    u32 low = val & 0xFFFFFFFF;
    u32 high = val >> 32;
    asm volatile("wrmsr" : : "c"(msr_id), "a"(low), "d"(high));
}

static inline u64 msr_read(u64 msr_id)
{
    u32 low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr_id));
    return ((u64)high << 32) | low;
}
