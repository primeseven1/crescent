#pragma once

#include <crescent/types.h>

#define likely(c) __builtin_expect(!!(c), 1)
#define unlikely(c) __builtin_expect(!!(c), 0)

#define asmlinkage __attribute__((sysv_abi))

#define E2BIG 1
#define EINVAL 2
#define ENOSYS 3
#define EBUSY 4
#define EADDRNOTAVAIL 5

extern const u8 _kernel_start_paddr;
extern const u8 _kernel_start_vaddr;
extern const u8 _kernel_readonly_start_paddr;
extern const u8 _kernel_readonly_start_vaddr;
extern const u8 _kernel_readonly_end_paddr;
extern const u8 _kernel_readonly_end_vaddr;
extern const u8 _kernel_end_paddr;
extern const u8 _kernel_end_vaddr;
