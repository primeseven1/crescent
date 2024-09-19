#include <crescent/common.h>
#include <crescent/asm/errno.h>
#include <crescent/core/panic.h>
#include <crescent/core/locking.h>
#include <crescent/core/cpu.h>
#include <crescent/core/printk.h>
#include <crescent/mm/vm_zone.h>
#include <crescent/mm/zone.h>
#include <crescent/mm/hhdm.h>
#include <crescent/lib/string.h>

#define MAX_4K_TOP_LEVEL_ENTRIES 0x8000000ul

struct vm_area {
    void* base;
    void* end;
    bool locked, free;
};

struct vm_zone {
    struct vm_area* area;
    unsigned long page_count;
    unsigned int gfp_flags;
    u8* map;
    unsigned long waiters;
    spinlock_t lock;
};

static void* _alloc_vpages(struct vm_zone* zone, unsigned long page_count) {
    unsigned long consecutive_free = 0;
    unsigned long start_idx = 0;

    for (unsigned long i = 0; i < zone->page_count; i++) {
        size_t byte_index = i / 8;
        u8 bit_index = i % 8;

        if (!(zone->map[byte_index] & (1 << bit_index))) {
            if (consecutive_free == 0)
                start_idx = i;

            consecutive_free++;

            if (consecutive_free == page_count) {
                for (unsigned long j = start_idx; j < start_idx + page_count; j++) {
                    byte_index = j / 8;
                    bit_index = j % 8;
                    zone->map[byte_index] |= (1 << bit_index);
                }

                return (u8*)zone->area->base + (start_idx * 0x1000);
            }

            continue;
        }

        consecutive_free = 0;
    }

    return NULL;
}

static void _reserve_vpages(struct vm_zone* zone, void* addr, unsigned long page_count) {
    unsigned long start_idx = ((uintptr_t)addr - (uintptr_t)zone->area->base) / 0x1000;
    for (unsigned long i = start_idx; i < start_idx + page_count; i++) {
        size_t byte_index = i / 8;
        u8 bit_index = i % 8;
        zone->map[byte_index] |= (1 << bit_index);
    }
}

static void _free_vpages(struct vm_zone* zone, void* addr, unsigned long page_count) {
    unsigned long start_idx = ((uintptr_t)addr - (uintptr_t)zone->area->base) / 0x1000;
    for (unsigned long i = start_idx; i < start_idx + page_count; i++) {
        size_t byte_index = i / 8;
        u8 bit_index = i % 8;
        zone->map[byte_index] &= ~(1 << bit_index);
    }
}

static bool are_last_n_pages_free(struct vm_zone* zone, unsigned long n) {
    n = (zone->page_count < n) ? zone->page_count : n;
    unsigned long start_idx = zone->page_count - n;

    for (unsigned long i = start_idx; i < zone->page_count; i++) {
        size_t byte_index = i / 8;
        u8 bit_index = i % 8;

        if (zone->map[byte_index] & (1 << bit_index))
            return false;
    }

    return true;
}

#define KERNEL_VM_ZONE_COUNT 256

static struct vm_zone** kernel_vm_zones;
static spinlock_t kernel_vm_zones_lock = SPINLOCK_INITIALIZER;

static struct vm_area kernel_vm_areas[KERNEL_VM_ZONE_COUNT];
static spinlock_t kernel_vm_areas_lock = SPINLOCK_INITIALIZER;

static struct vm_area* alloc_virtual_area(void) {
    struct vm_area* ret = NULL;

    unsigned long flags;
    spinlock_lock_irq_save(&kernel_vm_areas_lock, &flags);

    for (size_t i = 0; i < ARRAY_SIZE(kernel_vm_areas); i++) {
        if (kernel_vm_areas[i].free) {
            kernel_vm_areas[i].free = false;
            ret = &kernel_vm_areas[i];
            break;
        }
    }

    spinlock_unlock_irq_restore(&kernel_vm_areas_lock, &flags);
    return ret;
}

static void free_virtual_area(struct vm_area* area) {
    unsigned long flags;
    spinlock_lock_irq_save(&kernel_vm_areas_lock, &flags);
    if (!area->locked)
        area->free = true;
    spinlock_unlock_irq_restore(&kernel_vm_areas_lock, &flags);
}

static struct vm_zone* get_zone_from_gfp(unsigned int gfp_flags, 
        unsigned long start_index, unsigned long* index) {
    struct vm_zone* ret = NULL;

    unsigned long lock_flags;
    spinlock_lock_irq_save(&kernel_vm_zones_lock, &lock_flags);

    for (unsigned long i = start_index; i < KERNEL_VM_ZONE_COUNT; i++) {
        if (!kernel_vm_zones[i])
            continue;
        if (kernel_vm_zones[i]->gfp_flags == gfp_flags) {
            *index = i;
            ret = kernel_vm_zones[i];
            break;
        }
    }

    spinlock_unlock_irq_restore(&kernel_vm_zones_lock, &lock_flags);
    return ret;
}

static struct vm_zone* get_zone_from_addr(void* addr, unsigned long* index) {
    struct vm_zone* ret = NULL;

    unsigned long lock_flags;
    spinlock_lock_irq_save(&kernel_vm_zones_lock, &lock_flags);

    for (unsigned long i = 0; i < KERNEL_VM_ZONE_COUNT; i++) {
        if (!kernel_vm_zones[i])
            continue;

        size_t zone_size = kernel_vm_zones[i]->page_count * 0x1000;
        if (addr >= kernel_vm_zones[i]->area->base && 
                (u8*)addr < (u8*)kernel_vm_zones[i]->area->base + zone_size) {
            *index = i;
            ret = kernel_vm_zones[i];
            break;
        }
    }

    spinlock_unlock_irq_restore(&kernel_vm_zones_lock, &lock_flags);
    return ret;
}

static unsigned long add_vm_zone(struct vm_zone* zone) {
    unsigned long ret = ULONG_MAX;

    unsigned long lock_flags;
    spinlock_lock_irq_save(&kernel_vm_zones_lock, &lock_flags);

    for (unsigned long i = 0; i < KERNEL_VM_ZONE_COUNT; i++) {
        if (!kernel_vm_zones[i]) {
            kernel_vm_zones[i] = zone;
            ret = i;
            break;
        }
    }

    spinlock_unlock_irq_restore(&kernel_vm_zones_lock, &lock_flags);
    return ret;
}

static int delete_vm_zone(unsigned long index) {
    if (index >= KERNEL_VM_ZONE_COUNT)
        return -EOVERFLOW;

    unsigned long lock_flags;
    spinlock_lock_irq_save(&kernel_vm_zones_lock, &lock_flags);
    kernel_vm_zones[index] = NULL;
    spinlock_unlock_irq_restore(&kernel_vm_zones_lock, &lock_flags);
    return 0;
}

static struct vm_zone* create_vm_zone(unsigned long page_count, unsigned int gfp_flags) {
    if (page_count == 0 || page_count > MAX_4K_TOP_LEVEL_ENTRIES)
        return NULL;

    struct vm_area* area = alloc_virtual_area();
    if (!area)
        return NULL;

    struct vm_zone* zone = alloc_page(GFP_ZONE_NORMAL);
    if (!zone)
        return NULL;
    zone = hhdm_virtual(zone);

    size_t map_size = page_count / 8 + 1;
    u8* map = alloc_pages(GFP_ZONE_NORMAL, map_size);
    if (!map) {
        free_virtual_area(area);
        free_page(hhdm_physical(zone));
        return NULL;
    }
    map = hhdm_virtual(map);
    memset(map, 0, map_size);

    zone->area = area;
    zone->page_count = page_count;
    zone->gfp_flags = gfp_flags;
    zone->waiters = 0;
    zone->map = map;
    zone->lock = SPINLOCK_INITIALIZER;

    struct vm_ctx* vm_ctx = &_cpu()->vm_ctx;
    for (unsigned long i = 0; i < page_count; i++) {
        void* virtual = (u8*)zone->area->base + i * 0x1000;
        if (get_physaddr(vm_ctx, virtual) != (void*)-1)
            _reserve_vpages(zone, virtual, 1);
    }

    return zone;
}

static int resize_vm_zone(struct vm_zone* zone, long page_count) {
    if (page_count == 0)
        return 0;

    long new_page_count = zone->page_count + page_count;
    if (new_page_count < 0)
        return -EOVERFLOW;
    if ((page_count > 0 && (unsigned long)new_page_count <= zone->page_count) ||
            (page_count < 0 && (unsigned long)new_page_count >= zone->page_count))
        return -EOVERFLOW;
    if ((u8*)zone->area->base + new_page_count * 0x1000 >= (u8*)zone->area->end)
        return -E2BIG;

    size_t old_map_size = zone->page_count / 8 + 1;
    size_t new_map_size = new_page_count / 8 + 1;

    u8* new_map = alloc_pages(GFP_ZONE_NORMAL, get_order(new_map_size));
    if (!new_map)
        return -ENOMEM;
    new_map = hhdm_virtual(new_map);

    memset(new_map, 0, new_map_size);
    if (old_map_size > new_map_size)
        memcpy(new_map, zone->map, new_map_size);
    else
        memcpy(new_map, zone->map, old_map_size);

    free_pages(hhdm_physical(zone->map), get_order(old_map_size));

    unsigned long old_page_count = zone->page_count;
    zone->map = new_map;
    zone->page_count = new_page_count;

    struct vm_ctx* vm_ctx = &_cpu()->vm_ctx;
    for (unsigned long i = old_page_count - 1; i < (unsigned long)new_page_count; i++) {
        void* virtual = (u8*)zone->area->base + i * 0x1000;
        if (get_physaddr(vm_ctx, virtual) != (void*)-1)
            _reserve_vpages(zone, virtual, 1);
    }

    return 0;
}

static void destroy_vm_zone(struct vm_zone* zone) {
    size_t map_size = zone->page_count / 8 + 1;
    free_pages(hhdm_physical(zone->map), get_order(map_size));
    free_virtual_area(zone->area);
    free_page(hhdm_physical(zone));
}

void* alloc_vpages(unsigned int gfp_flags, unsigned int order) {
    /* This will be the case for now, since no user pages are supported yet */
    if (!(gfp_flags & GFP_VM_KERNEL))
        return NULL;

    size_t alloc_size = 4096ul << order;
    size_t page_count = alloc_size / 0x1000;

    void* ret;
    unsigned long index = 0;

    struct vm_zone* zone;
    unsigned long zone_lock_flags;

    while (1) {
        zone = get_zone_from_gfp(gfp_flags, index, &index);
        if (!zone) {
            zone = create_vm_zone(page_count, gfp_flags);
            if (!zone) {
                printk("Failed to create a new memory zone\n");
                return NULL;
            }

            index = add_vm_zone(zone);
            if (unlikely(index == ULONG_MAX)) {
                destroy_vm_zone(zone);
                printk("Succesfully allocated a new virtual memory zone, but could not add it to the list.\n");
                return NULL;
            }
        }

        __atomic_add_fetch(&zone->waiters, 1, __ATOMIC_SEQ_CST);
        spinlock_lock_irq_save(&zone->lock, &zone_lock_flags);
        __atomic_sub_fetch(&zone->waiters, 1, __ATOMIC_SEQ_CST);

        ret = _alloc_vpages(zone, page_count);
        if (ret)
            goto out;

        while (1) {
            int err = resize_vm_zone(zone, page_count);
            if (err)
                break;

            printk("Resized the virtual memory zone (base %p) by %lu pages\n", 
                    zone->area->base, page_count);
            ret = _alloc_vpages(zone, page_count);
            if (ret)
                goto out;
        }

        /* 
         * Since the last code failed to get a free page, we need to create another 
         * or find another memory zone at a different index 
         */
        index++;
        spinlock_unlock_irq_restore(&zone->lock, &zone_lock_flags);
    }

out:
    spinlock_unlock_irq_restore(&zone->lock, &zone_lock_flags);
    return ret;
}

/* Try to either shrink or destroy the memory zone, returns true if the zone is destroyed */
static bool try_zone_shrink_or_destroy(struct vm_zone* zone, unsigned long zone_index) {
    if (are_last_n_pages_free(zone, 16)) {
        if (zone->page_count <= 16 && !zone->area->locked && 
                __atomic_load_n(&zone->waiters, __ATOMIC_SEQ_CST) == 0) {
            delete_vm_zone(zone_index);

            /* 
             * Since another thread can still get the lock before it's deleted, 
             * make sure that there are still no waiters 
             */
            if (__atomic_load_n(&zone->waiters, __ATOMIC_SEQ_CST) == 0) {
                void* base = zone->area->base;
                destroy_vm_zone(zone);
                printk("Virtual memory zone destroyed (base %p)\n", base);
                return true;
            }

            /* 
             * Hopefully this doesn't happen, since there isn't a good way to 
             * recover from this if the zone cannot be added back, chances are
             * you'll run out of memory before this even happens.
             */
            unsigned long index = add_vm_zone(zone);
            if (unlikely(index == ULONG_MAX)) {
                printk("Failed to add vm_zone struct after a failed attempt at destoying\n");
                panic("kernel_vm_zones list is in a bad state");
            }

            return false;
        }
    }

    long count = -16;
    int err = resize_vm_zone(zone, count);
    if (!err)
        printk("Resized virtual memory zone (base %p) by %li pages\n", zone->area->base, count);
    else
        printk("Error when resizing memory zone (base %p) err: %i\n", zone->area->base, err);

    return false;
}

void free_vpages(void* addr, unsigned int order) {
    unsigned long index;
    struct vm_zone* zone = get_zone_from_addr(addr, &index);
    if (!zone) {
        printk("Failed to get virtual memory zone from address %p!\n", addr);
        return;
    }

    size_t alloc_size = 4096ul << order;
    unsigned long page_count = alloc_size / 0x1000;

    unsigned long lock_flags;
    __atomic_add_fetch(&zone->waiters, 1, __ATOMIC_SEQ_CST);
    spinlock_lock_irq_save(&zone->lock, &lock_flags);
    __atomic_sub_fetch(&zone->waiters, 1, __ATOMIC_SEQ_CST);

    _free_vpages(zone, addr, page_count);
    bool zone_destroyed = try_zone_shrink_or_destroy(zone, index);

    if (likely(!zone_destroyed))
        spinlock_unlock_irq_restore(&zone->lock, &lock_flags);
}

void vm_zone_init(void) {
    kernel_vm_zones = alloc_page(GFP_ZONE_NORMAL);
    if (!kernel_vm_zones)
        panic("Failed to allocate kernel vm zone list");
    kernel_vm_zones = hhdm_virtual(kernel_vm_zones);
    for (size_t i = 0; i < KERNEL_VM_ZONE_COUNT; i++)
        kernel_vm_zones[i] = 0;

    unsigned long* top_level_pt = _cpu()->vm_ctx.ctx;
    unsigned long top_level_index = 256;

    for (size_t i = 0; i < ARRAY_SIZE(kernel_vm_areas); i++) {
        kernel_vm_areas[i].base = (void*)((top_level_index << 39) | 0xFFFF000000000000);
        kernel_vm_areas[i].end = (u8*)kernel_vm_areas[i].base + MAX_4K_TOP_LEVEL_ENTRIES * 0x1000;

        if (top_level_pt[top_level_index] & MMU_FLAG_PRESENT) {
            kernel_vm_areas[i].free = false;
            kernel_vm_areas[i].locked = true;
        } else {
            kernel_vm_areas[i].free = true;
            kernel_vm_areas[i].locked = false;
        }

        top_level_index++;
    }

    /* 
     * Create the first virtual memory zone. Lock the area so it cannot be freed.
     * Doing this also mitigates a CPU bug around executing code around the canonical
     * address boundary for AMD cpu's since it will find the first virtual memory area and
     * lock it.
     */
    struct vm_zone* zone = create_vm_zone(1, GFP_VM_KERNEL);
    if (!zone)
        panic("Failed to create the first virtual memory zone. This should not happen!");
    zone->area->locked = true;
}
