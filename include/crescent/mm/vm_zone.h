#pragma once

#include <crescent/types.h>
#include <crescent/core/locking.h>

enum vm_zone_flags {
    VM_READ_WRITE,
    VM_EXEC,
    VM_HUGE,
    VM_SHARED,
};

struct vm_zone {
    void* base;
    unsigned long pages;
    unsigned int flags;
    u8* map;
    spinlock_t lock;
};

void vm_zone_init(void);
struct vm_zone* create_vm_zone(void* base, unsigned long pages, unsigned int flags);
void destroy_vm_zone(struct vm_zone* zone);
