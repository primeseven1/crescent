#include <crescent/common.h>
#include <crescent/asm/errno.h>
#include <crescent/core/locking.h>
#include <crescent/mm/vm_zone.h>

#define MAX_TOP_LEVEL_4K_PAGES 0x1000000000
#define MAX_TOP_LEVEL_2M_PAGES 0x8000000

struct vm_zone {
    void* base;
    unsigned long page_count;
    unsigned int flags;
    u8* map;
    spinlock_t lock;
};

static int resize_vm_zone(struct vm_zone* zone, long page_count) {
    return -ENOSYS;
}

static struct vm_zone* create_vm_zone(unsigned long page_count, unsigned int gfp_flags) {
    return NULL;
}

static void destroy_vm_zone(struct vm_zone* zone) {
}

void* alloc_pages_virtual(unsigned int gfp_flags, unsigned int order) {
    return NULL;
}

void free_pages_virtual(void* addr, unsigned int order) {
}

void vm_zone_init(void) {
}
