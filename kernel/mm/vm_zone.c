#include <crescent/common.h>
#include <crescent/core/cpu.h>
#include <crescent/core/locking.h>
#include <crescent/mm/hhdm.h>
#include <crescent/mm/zone.h>
#include <crescent/mm/mmap.h>
#include <crescent/mm/vm_zone.h>
#include <crescent/core/printk.h>

static const u8* hhdm_start;
static const u8* hhdm_end;

struct vm_zone* create_vm_zone(void* base, unsigned long pages, unsigned int flags) {
    /* Not implemented */
    if (flags & VM_SHARED)
        return NULL;

    struct vm_zone* ret = alloc_page(GFP_ZONE_NORMAL);
    if (!ret)
        return NULL;
    ret = hhdm_virtual(ret);

    ret->base = base;
    ret->pages = pages;
    ret->flags = flags;

    size_t map_size = pages / 8 + 1;
    u8* map = alloc_pages(GFP_ZONE_NORMAL, get_order(map_size));
    if (!map) {
        free_page(hhdm_physical(ret));
        return NULL;
    }
    map = hhdm_virtual(map);
    ret->map = map;

    /* Placeholder code, for now */
    if (flags & VM_SHARED)
        ret->lock = SPINLOCK_INITIALIZER;

    return ret;
}

void destroy_vm_zone(struct vm_zone* zone) {
    size_t page_size = zone->flags & VM_HUGE ? 0x200000 : 0x1000;
    unsigned int page_size_order = get_order(page_size);
    size_t map_size = zone->pages / 8 + 1;
    unsigned int map_size_order = get_order(map_size);

    /* 
     * For shared mappings, this will need to be done differently 
     * but shared mappings are not implemented yet, so this is fine
     */
    struct vm_ctx* vm_ctx = &_cpu()->vm_ctx;

    /* Unmap and free every page in the zone */
    for (unsigned long i = 0; i < zone->pages; i++) {
        void* virtual = (u8*)zone->base + i * page_size;
        void* physical = get_physaddr(vm_ctx, virtual);
        if (physical != (void*)-1) {
            unmap_page(vm_ctx, virtual);
            free_pages(physical, page_size_order);
        }
    }

    free_pages(hhdm_physical(zone->map), map_size_order);
    free_page(hhdm_physical(zone));
}

void vm_zone_init(void) {
    hhdm_start = hhdm_virtual(NULL);
    hhdm_end = hhdm_start;

    struct cpu* cpu = _cpu();
    while (get_physaddr(&cpu->vm_ctx, hhdm_end) != (void*)-1)
        hhdm_end += 0x200000;
}
