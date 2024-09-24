#include <crescent/common.h>
#include <crescent/core/printk.h>
#include <crescent/core/panic.h>
#include <crescent/core/cpu.h>
#include <crescent/mm/mmap.h>
#include <crescent/mm/heap.h>
#include <crescent/lib/string.h>

#define HEAP_CHK_VALUE 0xdecafc0ffee

/* 
 * This is a temporary allocator, this is just a placeholder for now,
 * I plan on using a slab allocator later on. These functions just use kmmap
 * to allocate memory.
 */

void* kmalloc(size_t size, unsigned int gfp_flags) {
    if (size & 7)
        size = (size & (~7)) + 8;

    int unused_errno;
    size_t* alloc_info = kmmap(NULL, size + sizeof(size_t) * 2 + sizeof(u64), 
            MMU_FLAG_READ_WRITE | MMU_FLAG_NX, gfp_flags, &unused_errno);
    if (alloc_info == (void*)-1)
        return NULL;

    alloc_info[0] = size;
    alloc_info[1] = gfp_flags;

    size_t* ret = alloc_info + 2;
    u64* end = (u64*)((uintptr_t)ret + size);
    *end = HEAP_CHK_VALUE;

    return ret;
}

void* krealloc(void* addr, size_t new_size, unsigned int gfp_flags) {
    if (new_size & 7)
        new_size = (new_size & (~7)) + 8;

    size_t* old_alloc_info = (size_t*)addr - 2;

    size_t old_size = old_alloc_info[0];
    size_t old_gfp_flags = old_alloc_info[1];

    u64* addr_end = (u64*)((uintptr_t)addr + old_size);
    if (*addr_end != HEAP_CHK_VALUE)
        panic("Kernel heap corruption, check value: %lx", *addr_end);

    int errno;
    size_t* new_alloc_info = kmmap(NULL, 
            new_size + sizeof(size_t) * 2 + sizeof(u64),
            MMU_FLAG_READ_WRITE | MMU_FLAG_NX, gfp_flags, &errno);
    if (new_alloc_info == (void*)-1)
        return NULL;

    void* new_addr = new_alloc_info + 2;
    if (new_size < old_size)
        memcpy(addr, new_addr, new_size);
    else
        memcpy(addr, new_addr, old_size);

    addr_end = (u64*)((uintptr_t)new_addr + new_size);
    *addr_end = HEAP_CHK_VALUE;

    errno = kmunmap(old_alloc_info, old_size, old_gfp_flags, true);
    if (errno)
        printk("Error unmapping old address in krealloc, errno: %i\n", errno);

    return new_addr;
}

void kfree(void* addr) {
    size_t* alloc_info = (size_t*)addr - 2;

    size_t size = alloc_info[0];
    u64* end = (u64*)((uintptr_t)addr + size);
    if (*end != HEAP_CHK_VALUE)
        panic("Kernel heap corruption, check value: %lx", *end);

    unsigned int gfp_flags = alloc_info[1];
    int errno = kmunmap(alloc_info, size + sizeof(size_t) * 2 + sizeof(u64), gfp_flags, true);
    if (errno)
        printk(PL_CRIT "Failed to unmap page(s) in kfree, errno: %i\n", errno);
}
