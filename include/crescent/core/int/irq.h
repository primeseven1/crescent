#pragma once

#include <crescent/types.h>

static inline void local_irq_enable(void) {
    __asm__ volatile("sti" : : : "memory");
}

static inline void local_irq_disable(void) {
    __asm__ volatile("cli" : : : "memory");
}
