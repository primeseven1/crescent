#pragma once

#include <crescent/asm/msr.h>

struct cpu {
    struct cpu* self;
    unsigned long* vmm_ctx;
};

/**
 * @brief Set the GSBASE MSR to the pointer of the per-cpu data
 */
static inline void _cpu_set(struct cpu* cpu) {
    cpu->self = cpu;
    wrmsr(MSR_GS_BASE, (uintptr_t)cpu);
}

/**
 * @brief Get the value of the GSBASE MSR (the per-cpu data)
 * @return The address of the per-cpu data
 */
static inline struct cpu* _cpu(void) {
    struct cpu* ret;
    __asm__("movq %%gs:0, %0" : "=r"(ret));
    return ret;
}
