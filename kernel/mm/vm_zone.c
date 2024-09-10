#include <crescent/common.h>
#include <crescent/core/cpu.h>
#include <crescent/core/locking.h>
#include <crescent/core/printk.h>
#include <crescent/core/panic.h>
#include <crescent/mm/hhdm.h>
#include <crescent/mm/mmap.h>
#include <crescent/mm/vm_zone.h>
#include <crescent/lib/string.h>

#define USER_PRIVATE_START 1
#define USER_PRIVATE_END 127
#define USER_SHARED_START 128
#define USER_SHARED_END 255

#define KERNEL_PRIVATE_START 256
#define KERNEL_PRIVATE_END 383
#define KERNEL_SHARED_START 384
#define KERNEL_SHARED_END 510

#define MAX_TOP_LEVEL_4K_PAGES 0x1000000000
#define MAX_TOP_LEVEL_2M_PAGES 0x8000000

static void* get_free_virtual(bool user_space, bool shared) {
    /* This is not implemented yet */
    if (user_space || shared)
        return NULL;

    struct vm_ctx* ctx = &_cpu()->vm_ctx;
    uintptr_t ret = 0;

    unsigned long lock_flags;
    spinlock_lock_irq_save(&ctx->lock, &lock_flags);

    unsigned long* top_level_table = ctx->ctx;

    /* This code will need to change whenever shared and user level zones are implemented */
    unsigned long index = KERNEL_PRIVATE_START;
    unsigned long count = KERNEL_PRIVATE_END - index;

    while (count) {
        if (!(top_level_table[index] & MMU_FLAG_PRESENT) && 
                !(top_level_table[index] & MMU_FLAG_VM_ZONE_ALLOC))
            break;

        index++;
        count--;
    }

    if (count == 0)
        goto out;

    /* Adding this flag will prevent the same entry from being allocated if the page table isn't present */
    top_level_table[index] |= MMU_FLAG_VM_ZONE_ALLOC;

    /* 
     * If the 47th bit is set, we need to set the other 16 bits to 1 too, otherwise the returned 
     * address will be non-canonical, and that will cause a general protection fault
     */
    ret = index << 39;
    if (ret & (1ul << 47))
        ret |= 0xFFFF000000000000;
out:
    spinlock_unlock_irq_restore(&ctx->lock, &lock_flags);
    return (void*)ret;
}

static bool check_last_16_pages_free(struct vm_zone* zone) {
    unsigned long page_count = zone->page_count;

    unsigned long lock_flags;
    if (zone->flags & GFP_VM_SHARED)
        spinlock_lock_irq_save(&zone->lock, &lock_flags);

    bool ret;
    
    unsigned long pages_to_check = page_count < 16 ? page_count : 16;
    unsigned long start_page = page_count - pages_to_check;

    for (unsigned long i = start_page; i < page_count; ++i) {
        unsigned long byte_index = i / 8;
        unsigned long bit_index = i % 8;

        if (zone->map[byte_index] & (1 << bit_index)) {
            ret = false;
            goto out;
        }
    }

    ret = true;
out:
    if (zone->flags & GFP_VM_SHARED)
        spinlock_unlock_irq_restore(&zone->lock, &lock_flags);
    return ret;
}

static void* find_free_vm_addr(struct vm_zone* zone, unsigned long page_count) {
    unsigned long start_page = 0;
    unsigned long free_pages = 0;
    void* found_addr = NULL;

    size_t page_size = zone->flags & GFP_VM_HUGE ? 0x200000 : 0x1000;

    unsigned long lock_flags;
    if (!(zone->flags & GFP_VM_SHARED))
        spinlock_lock_irq_save(&zone->lock, &lock_flags);

    for (unsigned long i = 0; i < zone->page_count; ++i) {
        unsigned long byte_index = i / 8;
        unsigned long bit_index = i % 8;

        if (!(zone->map[byte_index] & (1 << bit_index))) {
            if (free_pages == 0)
                start_page = i;

            free_pages++;

            if (free_pages >= page_count) {
                found_addr = (u8*)zone->base + (start_page * page_size);

                for (unsigned long j = start_page; j < start_page + page_count; ++j) {
                    unsigned long alloc_byte_index = j / 8;
                    unsigned long alloc_bit_index = j % 8;
                    zone->map[alloc_byte_index] |= (1 << alloc_bit_index);
                }

                break;
            }
        } else {
            free_pages = 0;
        }
    }

    if (!(zone->flags & GFP_VM_SHARED))
        spinlock_unlock_irq_restore(&zone->lock, &lock_flags);
    return found_addr;
}

static void free_vm_addr(struct vm_zone* zone, void* addr, unsigned long page_count) {
    size_t page_size = (zone->flags & GFP_VM_HUGE) ? 0x200000 : 0x1000;
    unsigned long start_page = ((u8*)addr - (u8*)zone->base) / page_size;

    unsigned long lock_flags;
    if (!(zone->flags & GFP_VM_SHARED))
        spinlock_lock_irq_save(&zone->lock, &lock_flags);

    for (unsigned long i = start_page; i < start_page + page_count; ++i) {
        unsigned long byte_index = i / 8;
        unsigned long bit_index = i % 8;

        zone->map[byte_index] &= ~(1 << bit_index);
    }

    if (!(zone->flags & GFP_VM_SHARED))
        spinlock_unlock_irq_restore(&zone->lock, &lock_flags);
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

    if (zone->flags & GFP_VM_HUGE && new_page_count > MAX_TOP_LEVEL_2M_PAGES)
        return -E2BIG;
    else if (new_page_count > MAX_TOP_LEVEL_4K_PAGES)
        return -E2BIG;

    size_t old_map_size = zone->page_count / 8 + 1;
    size_t new_map_size = new_page_count / 8 + 1;
    u8* new_map = alloc_pages(GFP_ZONE_NORMAL, get_order(new_map_size));
    if (!new_map)
        return -ENOMEM;
    new_map = hhdm_virtual(new_map);
    memset(new_map, 0, new_map_size);

    if (old_map_size > new_map_size) {
        struct vm_ctx* vm_ctx = &_cpu()->vm_ctx;
        size_t page_size = zone->flags & GFP_VM_HUGE ? 0x200000 : 0x1000;

        /* Make sure pages that are being discarded are unmapped */
        for (unsigned long i = new_page_count - 1; i < zone->page_count; i++) {
            void* virtual = (u8*)zone->base + i * page_size;
            void* physical = get_physaddr(vm_ctx, virtual);
            if (physical != (void*)-1) {
                printk("WARNING: shrinking size of memory zone (base %p), but %p is still mapped\n",
                        zone->base, virtual);
                unmap_page(vm_ctx, virtual);
            }
        }

        memcpy(new_map, zone->map, new_map_size);
    } else {
        memcpy(new_map, zone->map, old_map_size);
    }

    free_pages(hhdm_physical(zone->map), get_order(old_map_size));
    zone->map = new_map;
    zone->page_count = new_page_count;
    return 0;
}

static struct vm_zone* create_vm_zone(unsigned long page_count, unsigned int gfp_flags) {
    struct vm_zone* ret = alloc_page(GFP_ZONE_NORMAL);
    if (!ret)
        return NULL;
    ret = hhdm_virtual(ret);

    if (gfp_flags & GFP_VM_HUGE && page_count > MAX_TOP_LEVEL_2M_PAGES)
        return NULL;
    else if (page_count > MAX_TOP_LEVEL_4K_PAGES)
        return NULL;

    void* base = get_free_virtual(gfp_flags & GFP_VM_USER, gfp_flags & GFP_VM_SHARED);
    if (!base)
        return NULL;
 
    size_t map_size = page_count / 8 + 1;
    u8* map = alloc_pages(GFP_ZONE_NORMAL, get_order(map_size));
    if (!map) {
        free_page(ret);
        return NULL;
    }
    map = hhdm_virtual(map);
    memset(map, 0, map_size);
    
    ret->base = base;
    ret->page_count = page_count;
    ret->flags = gfp_flags;
    ret->map = map;
    ret->lock = SPINLOCK_INITIALIZER;

    return ret;
}

static void destroy_vm_zone(struct vm_zone* zone) {
    size_t page_size = zone->flags & GFP_VM_HUGE ? 0x200000 : 0x1000;

    /* This needs to be done differently for shared mappings, but that's not implemented yet */
    struct vm_ctx* ctx = &_cpu()->vm_ctx;
    for (unsigned long i = 0; i < zone->page_count; i++) {
        void* virtual = (u8*)zone->base + i * page_size;
        void* physical = get_physaddr(ctx, virtual);
        if (unlikely(physical != (void*)-1)) {
            printk("WARNING: destroying virtual memory zone (base %p), but %p is still mapped",
                    zone->base, virtual);
            /* 
             * Cannot free this memory since we don't know if the physical memory is allocated 
             * by another CPU, or possibly even this CPU, but we know we can unmap it
             */
            unmap_page(ctx, virtual);
        }
    }

    size_t map_size = zone->page_count / 8 + 1;
    free_pages(hhdm_physical(zone->map), get_order(map_size));
    free_page(hhdm_physical(zone));
}

static int add_private_vm_zone(struct vm_zone* zone) {
    struct cpu* cpu = _cpu();
    for (unsigned long i = 0; i < cpu->private_vm_zones.zone_count; i++) {
        if (!cpu->private_vm_zones.zones[i]) {
            cpu->private_vm_zones.zones[i] = zone;
            return i;
        }
    }

    return -E2BIG;
}

static int delete_private_vm_zone(unsigned int index) {
    struct cpu* cpu = _cpu();
    if (index >= cpu->private_vm_zones.zone_count)
        return -EOVERFLOW;

    cpu->private_vm_zones.zones[index] = NULL;
    return 0;
}

/* 
 * Get a virtual memory zone based on some flags, start_index is for where to start searching.
 * The index of the zone will be stored in index
 */
static struct vm_zone* get_private_vm_zone_from_gfp(unsigned int gfp_flags, 
        unsigned long start_index, unsigned long* index) {
    struct cpu* cpu = _cpu();
    if (unlikely(!cpu->private_vm_zones.zones))
        return NULL;

    for (unsigned long i = start_index; i < cpu->private_vm_zones.zone_count; i++) {
        if (!cpu->private_vm_zones.zones[i])
            continue;
        if (cpu->private_vm_zones.zones[i]->flags == gfp_flags) {
            *index = i;
            return cpu->private_vm_zones.zones[i];
        }
    }

    return NULL;
}

/* Retrive a virtual memory zone based on a virtual address. The index of the zone is stored in index */
static struct vm_zone* get_private_vm_zone_from_addr(void* addr, unsigned long* index) {
    struct cpu* cpu = _cpu();
    if (unlikely(!cpu->private_vm_zones.zones))
        return NULL;

    for (unsigned long i = 0; i < cpu->private_vm_zones.zone_count; i++) {
        if (!cpu->private_vm_zones.zones[i])
            continue;

        size_t page_size = cpu->private_vm_zones.zones[i]->flags & GFP_VM_HUGE ? 0x200000 : 0x1000;
        uintptr_t start = (uintptr_t)cpu->private_vm_zones.zones[i]->base;
        uintptr_t end =  start + cpu->private_vm_zones.zones[i]->page_count * page_size;

        if ((uintptr_t)addr >= start && (uintptr_t)addr < end) {
            *index = i;
            return cpu->private_vm_zones.zones[i];
        }
    }

    return NULL;
}

void* alloc_pages_virtual(unsigned int gfp_flags, unsigned int order) {
    size_t alloc_size = 4096ul << order;
    size_t page_size = gfp_flags & GFP_VM_HUGE ? 0x200000 : 0x1000;

    /* For 4K pages, the else block will always run */
    unsigned long page_count;
    if (alloc_size & (page_size - 1))
        page_count = alloc_size / page_size + 1;
    else
        page_count = alloc_size / page_size;

    /* Clear any invalid flags */
    gfp_flags &= ~(GFP_ZONE_DMA | GFP_ZONE_DMA32 | GFP_ZONE_NORMAL);
    /* Not implemeneted */
    if (gfp_flags & GFP_VM_SHARED)
        return NULL;

    void* ret;
    unsigned long zone_index = 0;
    struct vm_zone* zone; 

    while (1) {
        /* Start searching for a zone at start_index in case a zone cannot be expanded */
        zone = get_private_vm_zone_from_gfp(gfp_flags, zone_index, &zone_index);
        if (!zone) {
            /* No zone, so try creating one with the flags we want */
            zone = create_vm_zone(page_count, gfp_flags);
            if (!zone)
                return NULL;
            printk("Created virtual memory zone, base: %p, flags: %u\n", zone->base, gfp_flags);
            add_private_vm_zone(zone);
        }

        ret = find_free_vm_addr(zone, page_count);
        if (ret)
            break;

        /* Try resizing, if that fails, Then a new zone has to be created */
        int err = resize_vm_zone(zone, page_count);
        if (!err) {
            printk("Resized memory zone at base %p by %lu page(s)\n", zone->base, page_count);
            ret = find_free_vm_addr(zone, page_count);
            if (ret)
                break;

            printk("ERROR: The virtual memory zone was successfully resized, but didn't return a valid pointer. This is a bug.\n");
            return NULL;
        }

        zone = NULL;
    }

    return ret;
}

void free_pages_virtual(void* addr, unsigned int order) {
    unsigned long index;
    struct vm_zone* zone = get_private_vm_zone_from_addr(addr, &index);
    if (!zone) {
        printk("WARNING: Failed to get virtual memory zone (%p)", addr);
        return;
    }

    size_t alloc_size = 4096ul << order;
    size_t page_size = zone->flags & GFP_VM_HUGE ? 0x200000 : 0x1000;

    /* For 4K pages, the else block will always run */
    unsigned long page_count = alloc_size / page_size;
    if (alloc_size & (page_size - 1))
        page_count = alloc_size / page_size + 1;
    else
        page_count = alloc_size / page_size;

    /* Make sure that the amount of pages being freed doesn't exceed the zone */
    unsigned long start_page = ((u8*)addr - (u8*)zone->base) / page_size;
    unsigned long pages_remaining = zone->page_count - start_page;
    if (page_count > pages_remaining) {
        printk("WARNING: Tried to free more pages than available (%p)\n", addr);
        return;
    }

    free_vm_addr(zone, addr, page_count);
    bool free = check_last_16_pages_free(zone);
    if (free) {
        if (zone->page_count < 16) {
            destroy_vm_zone(zone);
            delete_private_vm_zone(index);
            printk("Destroyed virtual memory zone at base %p\n", zone->base);
            return;
        }

        resize_vm_zone(zone, -16);
        printk("Resized memory zone at base %p by %i pages\n", zone->base, -16);
    }
}

void vm_zone_init(void) {
    struct cpu* cpu = _cpu();
    struct vm_zone** zones = alloc_page(GFP_ZONE_NORMAL);
    if (!zones)
        panic("Failed to initiaze virtual memory zones");

    zones = hhdm_virtual(zones);
    memset(zones, 0, 0x1000);
    cpu->private_vm_zones.zones = zones;
    cpu->private_vm_zones.zone_count = 128;
}
