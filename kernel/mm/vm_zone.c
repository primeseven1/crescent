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

#define PML4_MAX_4K_PAGES 0x8000000ul
#define KERNEL_ADDRESS_SPACE_START ((void*)((256ul << 39) | 0xFFFF000000000000))

struct vm_zone {
    struct {
        void* base;
        void* end;
    } area;
    unsigned long page_count;
    gfp_t gfp_flags;
    u8* map;
    spinlock_t lock;
};

/* Check if the last n pages are free in a zone, this is used for shrinking memory zones */
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

/* Used for growing/shrinking memory zones when needed */
static int resize_vm_zone(struct vm_zone* zone, long page_count) {
    if (page_count == 0)
        return 0;

    long new_page_count = zone->page_count + page_count;
    if (new_page_count < 0)
        return -EOVERFLOW;
    if ((page_count > 0 && (unsigned long)new_page_count <= zone->page_count) ||
            (page_count < 0 && (unsigned long)new_page_count >= zone->page_count))
        return -EOVERFLOW;
    if ((u8*)zone->area.base + new_page_count * 0x1000 >= (u8*)zone->area.end)
        return -E2BIG;

    size_t old_map_size = zone->page_count & 7 ? zone->page_count / 8 + 1 : zone->page_count / 8;
    size_t new_map_size = new_page_count & 7 ? new_page_count / 8 + 1 : new_page_count / 8;

    u8* new_map;
    if (new_page_count) {
        new_map = alloc_pages(GFP_PM_ZONE_NORMAL, get_order(new_map_size));
        if (!new_map)
            return -ENOMEM;
        new_map = hhdm_virtual(new_map);

        memset(new_map, 0, new_map_size);
        if (old_map_size > new_map_size)
            memcpy(new_map, zone->map, new_map_size);
        else
            memcpy(new_map, zone->map, old_map_size);
    } else {
        new_map = NULL;
    }

    if (zone->map)
        free_pages(hhdm_physical(zone->map), get_order(old_map_size));

    zone->map = new_map;
    zone->page_count = new_page_count;

    return 0;
}

/* Helper function for alloc_vpages */
static void* _alloc_vpages(struct vm_zone* zone, unsigned long page_count, bool align_2m) {
    unsigned long consecutive_free = 0;
    unsigned long start_idx = 0;

    for (unsigned long i = 0; i < zone->page_count; i++) {
        if (align_2m && consecutive_free == 0 && i % 512 != 0)
            continue;

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

                return (u8*)zone->area.base + (start_idx * 0x1000);
            }

            continue;
        }

        consecutive_free = 0;
    }

    return NULL;
}

/* Unused for now */
__attribute__((unused))
static int _reserve_vpages(struct vm_zone* zone, void* addr, unsigned long page_count) {
    unsigned long start_idx = ((uintptr_t)addr - (uintptr_t)zone->area.base) / 0x1000;
    if (start_idx + page_count >= zone->page_count)
        return -EOVERFLOW;

    size_t byte_index;
    u8 bit_index;

    /* First make sure all pages can be reserved */
    for (unsigned long i = start_idx; i < start_idx + page_count; i++) {
        byte_index = i / 8;
        bit_index = i % 8;
        if (zone->map[byte_index] & (1 << bit_index))
            return -EADDRINUSE;
    }

    /* Now reserve the pages */
    for (unsigned long i = start_idx; i < start_idx + page_count; i++) {
        byte_index = i / 8;
        bit_index = i % 8;
        zone->map[byte_index] |= (1 << bit_index);
    }

    return 0;
}

/* Helper function for free_vpages */
static void _free_vpages(struct vm_zone* zone, void* addr, unsigned long page_count) {
    unsigned long start_idx = ((uintptr_t)addr - (uintptr_t)zone->area.base) / 0x1000;
    for (unsigned long i = start_idx; i < start_idx + page_count; i++) {
        size_t byte_index = i / 8;
        u8 bit_index = i % 8;
        zone->map[byte_index] &= ~(1 << bit_index);
    }
}

static struct vm_zone kernel_vm_zones[256];

/* 
 * Get zone from gfp flags, index should contain the initial index, 
 * and then the index of the zone being returned is stored back to index.
 */
static struct vm_zone* get_vm_zone_gfp(gfp_t gfp_flags, size_t* index) {
    if (gfp_flags & GFP_VM_KERNEL) {
        for (size_t i = *index; i < ARRAY_SIZE(kernel_vm_zones); i++) {
            if ((gfp_flags & GFP_VM_EXEC) && 
                    unlikely(!(kernel_vm_zones[i].gfp_flags & GFP_VM_EXEC)))
                continue;

            *index = i;
            return &kernel_vm_zones[i];
        }
    }

    return NULL;
}

/* 
 * Get zone from an address. Unlike get_vm_zone_gfp, the initial value stored at index
 * is ignored, but the index of the returned zone is stored at index.
 */
static struct vm_zone* get_vm_zone_addr(void* addr, size_t* index) {
    if (addr >= KERNEL_ADDRESS_SPACE_START) {
        for (size_t i = 0; i < ARRAY_SIZE(kernel_vm_zones); i++) {
            if (addr >= kernel_vm_zones[i].area.base && addr < kernel_vm_zones[i].area.end) {
                *index = i;
                return &kernel_vm_zones[i];
            }
        }
    }

    return NULL;
}

void* alloc_vpages(gfp_t gfp_flags, unsigned int order) {
    bool large_page = gfp_flags & GFP_VM_LARGE_PAGE;
    size_t page_count = (4096ul << order) / 0x1000;
    if (large_page) {
        page_count = ROUND_UP(page_count, 512);
        gfp_flags &= ~(GFP_VM_LARGE_PAGE);
    }
   
    struct vm_zone* zone;
    unsigned long zone_lock_flags;
    size_t zone_index = 0;

    void* ret;
    while (1) {
        /* Try getting a zone with the GFP flags */
        zone = get_vm_zone_gfp(gfp_flags, &zone_index);
        if (unlikely(!zone))
            return NULL;
        
        /* Zone found, try allocating from it */
        spinlock_lock_irq_save(&zone->lock, &zone_lock_flags);
        ret = _alloc_vpages(zone, page_count, large_page);
        if (ret)
            goto out;

        /* Failed to allocate, try resizing */
        while (1) {
            unsigned long resize_count = page_count + 256;
            int err = resize_vm_zone(zone, resize_count);
            if (err) {
                resize_count -= 256;
                err = resize_vm_zone(zone, resize_count);
                if (err)
                    break;
            }

            printk("Resized virtual memory zone (base %p) by %lu pages\n", 
                    zone->area.base, resize_count);
            ret = _alloc_vpages(zone, page_count, large_page);
            if (ret)
                goto out;
        }

        /* Error resizing the memory zone, try with another zone */
        zone_index++;
        spinlock_unlock_irq_restore(&zone->lock, &zone_lock_flags);
    }

out:
    spinlock_unlock_irq_restore(&zone->lock, &zone_lock_flags);
    return ret;
}

void free_vpages(void* addr, unsigned int order) {
    unsigned long unused_index;
    struct vm_zone* zone = get_vm_zone_addr(addr, &unused_index);
    if (!zone) {
        printk(PL_ERR "Failed to get virtual memory zone from address %p!\n", addr);
        return;
    }

    size_t alloc_size = 4096ul << order;
    unsigned long page_count = alloc_size / 0x1000;

    unsigned long lock_flags;
    spinlock_lock_irq_save(&zone->lock, &lock_flags);

    /* Free the pages and then see if the zone can be shrinked */
    _free_vpages(zone, addr, page_count);
    if (zone->page_count > 512 && are_last_n_pages_free(zone, 512)) {
        long s_zone_page_count = (long)zone->page_count;
        if (s_zone_page_count < 0) {
            printk(PL_ERR "Tried to shrink the virtual memory zone at base %p, but there was signed integer overflow\n",
                    zone->area.base);
            goto out;
        }

        long count = -512;
        int err = resize_vm_zone(zone, count);
        if (!err)
            printk("Resized virtual memory zone (base %p) by %li pages\n",
                    zone->area.base, count);
        else
            printk(PL_ERR "Error when resizing memory zone (base %p) err: %i\n", 
                    zone->area.base, err);
    }

out:
    spinlock_unlock_irq_restore(&zone->lock, &lock_flags);
}

void vm_zones_init(void) {
    unsigned long* page_table = _cpu()->vm_ctx.ctx;

    size_t present_count = 0;
    for (size_t i = 0; i < ARRAY_SIZE(kernel_vm_zones); i++) {
        if (unlikely(page_table[256 + i] & MMU_FLAG_PRESENT)) {
            kernel_vm_zones[i].area.base = NULL;
            kernel_vm_zones[i].area.end = NULL;
            kernel_vm_zones[i].gfp_flags = 0;
            present_count++;
            continue;
        }

        /* 
         * To get the base, simply just reverse the calculation for the top page table index
         * and then bitwise OR it with 0xFFFF000000000000 to make the address canonical.
         */
        kernel_vm_zones[i].area.base = (void*)(((256ul + i) << 39) | 0xFFFF000000000000);
        kernel_vm_zones[i].area.end = (u8*)kernel_vm_zones[i].area.base + PML4_MAX_4K_PAGES * 0x1000;
        kernel_vm_zones[i].page_count = 0;

        /* 
         * Have the first level 4 entry be marked as not executable 
         * for a mitigiation of an AMD CPU bug. Blocking the whole entry
         * is very overkill, but it's a simple mitigation.
         */
        if (unlikely(i == 0))
            kernel_vm_zones[i].gfp_flags = GFP_VM_KERNEL;
        else
            kernel_vm_zones[i].gfp_flags = GFP_VM_KERNEL | GFP_VM_EXEC;

        kernel_vm_zones[i].lock = SPINLOCK_INITIALIZER;
    }

    printk("Initialized virtual memory zones, ranges reserved by loader: %zu\n", present_count);
}
