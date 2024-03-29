#pragma once

#include <crescent/types.h>

/* Internal use */
static inline void _real_cpuid(u32* eax, u32* ebx, u32* ecx, u32* edx)
{
    asm volatile("cpuid" 
                 : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) 
                 : "0"(*eax), "2"(*ecx) 
                 : "memory");
}

static inline void cpuid_all(unsigned int page, int count, 
    u32* eax, u32* ebx, u32* ecx, u32* edx)
{
    *eax = page;
    *ecx = count;
    _real_cpuid(eax, ebx, ecx, edx);
}
