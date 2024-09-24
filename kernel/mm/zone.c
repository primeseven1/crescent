#include <crescent/common.h>
#include <crescent/core/limine.h>
#include <crescent/core/locking.h>
#include <crescent/core/panic.h>
#include <crescent/core/printk.h>
#include <crescent/lib/string.h>
#include <crescent/mm/zone.h>
#include <crescent/mm/hhdm.h>

static inline bool full_overlap_check(void* base1, void* top1, void* base2, void* top2) {
    return ((base1 >= base2 && base1 < top2) && (top1 > base2 && top1 <= top2));
}

static void* get_last_usable(void) {
    u8* ret = NULL;
    struct limine_mmap_response* mmap = g_limine_mmap_request.response;

    for (u64 i = 0; i < mmap->entry_count; i++) {
        if (mmap->entries[i]->type == LIMINE_MMAP_USABLE && 
                (u8*)mmap->entries[i]->base + mmap->entries[i]->len - 1 > ret)
            ret = (u8*)mmap->entries[i]->base + mmap->entries[i]->len - 1;
    }

    return ret;
}

static bool is_region_usable(void* base, size_t size) {
    void* top = (u8*)base + size;

    struct limine_mmap_response* mmap = g_limine_mmap_request.response;
    for (u64 i = 0; i < mmap->entry_count; i++) {
        if (mmap->entries[i]->type != LIMINE_MMAP_USABLE)
            continue;

        void* mmap_top = (u8*)mmap->entries[i]->base + mmap->entries[i]->len;
        if (full_overlap_check(base, top, mmap->entries[i]->base, mmap_top))
            return true;

        if ((mmap->entries[i]->base >= base && mmap->entries[i]->base < top) ||
                (mmap_top > base && mmap_top <= top)) {
            void* overlap_bottom = base;
            if (mmap->entries[i]->base >= base && mmap->entries[i]->base < top)
                overlap_bottom = mmap->entries[i]->base;

            void* overlap_top = top;
            if (mmap_top > base && mmap_top <= top)
                overlap_top = mmap_top;

            size_t overlap_size = (uintptr_t)overlap_top - (uintptr_t)overlap_bottom;
            size -= overlap_size;
            if (size == 0)
                return true;
        }
    }

    return false;
}

struct zone {
    void* base;
    unsigned int type;
    size_t size;
    size_t real_size;
    u8 layers;
    u8* map;
    spinlock_t lock;
};

static inline void _alloc_block(u8* map, unsigned long offset, unsigned long block) {
    size_t byte_index = (offset + block - 1) / 8;
    unsigned int bit_index = (offset + block - 1) % 8;
    map[byte_index] |= (1 << bit_index);
}

static inline void _free_block(u8* map, unsigned long offset, unsigned long block) {
    size_t byte_index = (offset + block - 1) / 8;
    unsigned int bit_index = (offset + block - 1) % 8;
    map[byte_index] &= ~(1 << bit_index);
}

static inline bool _is_block_free(u8* map, unsigned long offset, unsigned long block) {
    size_t byte_index = (offset + block - 1) / 8;
    unsigned int bit_index = (offset + block - 1) % 8;
    return ((map[byte_index] & (1 << bit_index)) == 0);
}

static int alloc_block(struct zone* zone, u8 layer, unsigned long block) {
    if (layer >= zone->layers)
        return -ERANGE;

    unsigned long num_blocks = 1ul << layer;
    if (block >= num_blocks)
        return -EADDRNOTAVAIL;
    if (!_is_block_free(zone->map, num_blocks, block))
        return -EADDRINUSE;

    _alloc_block(zone->map, num_blocks, block);

    unsigned long tmp = block;
    u8 tmp2 = layer;

    /* Allocate the bigger blocks */
    while (layer--) {
        num_blocks = 1ul << layer;
        block >>= 1;
        _alloc_block(zone->map, num_blocks, block);
    }

    block = tmp;
    layer = tmp2;

    /* Allocate the smaller blocks */
    unsigned long times = 2;
    while (++layer < zone->layers) {
        num_blocks = 1ul << layer;
        block <<= 1;
        for (unsigned long i = 0; i < times; i++)
            _alloc_block(zone->map, num_blocks, block + i);
        times *= 2;
    }

    return 0;
}

static int free_block(struct zone* zone, u8 layer, unsigned long block) {
    if (layer >= zone->layers)
        return -ERANGE;

    unsigned long num_blocks = 1ul << layer;
    if (block >= num_blocks)
        return -EADDRNOTAVAIL;
    if (_is_block_free(zone->map, num_blocks, block))
        return -EFAULT;

    unsigned long tmp = block;
    u8 tmp2 = layer;

    /* Start out by freeing the bigger blocks, merging blocks as needed */
    while (1) {
        _free_block(zone->map, num_blocks, block);

        /* Layer 0 has no buddy */
        if (layer == 0)
            break;

        /* If the buddy isn't free, this part of the free process is done */
        unsigned long buddy = block & 1 ? block - 1 : block + 1;
        if (!_is_block_free(zone->map, num_blocks, buddy))
            break;

        layer--;
        num_blocks = 1ul << layer;
    }

    block = tmp;
    layer = tmp2;

    /* Now finish by freeing the smaller blocks */
    unsigned long count = 2;
    while (++layer < zone->layers) {
        num_blocks = 1ul << layer;
        block <<= 1;
        for (unsigned long i = 0; i < count; i++)
            _free_block(zone->map, num_blocks, block + i);
        count *= 2;
    }

    return 0;
}

static bool is_block_free(struct zone* zone, u8 layer, unsigned long block) {
    if (layer >= zone->layers)
        return false;
    unsigned long num_blocks = 1ul << layer;
    if (block >= num_blocks)
        return false;

    return _is_block_free(zone->map, num_blocks, block);
}

static unsigned long round_pow2_up(unsigned long base, unsigned long x) {
    if (x < base || base == 0)
        return base;

    unsigned long ret = base;
    while (ret < x)
        ret <<= 1;
    return ret;
}

static u8 get_layer_num(size_t zone_size, size_t alloc_size) {
    u8 layer = 0;
    while (zone_size > alloc_size) {
        zone_size /= 2;
        layer++;
    }
    return layer;
}


static unsigned long find_first_free(u8* map, u8 layer) {
    unsigned long num_blocks = 1ul << layer;
    unsigned long i = 0;

    const unsigned long ulong_bits = sizeof(unsigned long) * 8;

    /* First, test 1 bit at a time until alignment */
    while (i < num_blocks && (num_blocks + i - 1) % ulong_bits) {
        size_t byte_index = (num_blocks + i - 1) / 8;
        unsigned int bit_index = (num_blocks + i - 1) % 8;
        
        if (!(map[byte_index] & (1ul << bit_index)))
            return i;

        i++;
    }

    /* Now test several bits at a time */
    while (i + ulong_bits < num_blocks) {
        size_t byte_index = (num_blocks + i - 1) / 8;
        unsigned long* bitmap_ulong = (unsigned long*)(map + byte_index);

        if (*bitmap_ulong != ULONG_MAX) {
            for (unsigned long bit = 0; bit < ulong_bits; bit++) {
                if (((*bitmap_ulong & (1ul << bit)) == 0))
                    return i + bit;
            }
        }

        i += ulong_bits;
    }

    /* Now test the rest of the bits 1 at a time */
    while (i < num_blocks) {
        size_t byte_index = (num_blocks + i - 1) / 8;
        unsigned int bit_index = (num_blocks + i - 1) % 8;
        
        if ((map[byte_index] & (1ul << bit_index)) == 0)
            return i;

        i++;
    }

    return ULONG_MAX;
}

static inline bool crosses_block_boundary(void* addr, size_t size, size_t block_size) {
    uintptr_t block_start = (uintptr_t)addr & ~(block_size - 1);
    return (uintptr_t)addr + size > block_start + block_size;
}

static int memblock_reserve(struct zone* zone, void* base, size_t size) {
    void* zone_top = (u8*)zone->base + zone->size;
    void* addr_top = (u8*)base + size;

    /* Now make sure the address is within the zone */
    if (addr_top <= zone->base || base >= zone_top)
        return -EADDRNOTAVAIL;

    /* If it goes outside of the zone, reserve what can be reserved */
    if (base < zone->base)
        base = zone->base;
    if (addr_top > zone_top)
        addr_top = zone_top;
    size = (uintptr_t)addr_top - (uintptr_t)base;

    /* So that alignments work correctly, do the allocation as if the base is 0 */
    base = (u8*)base - (uintptr_t)zone->base;

    /* Do some alignments so that calculations work correctly */
    size_t prev_size = size;
    size = round_pow2_up(zone->size >> (zone->layers - 1), size);
    void* prev_base = base;
    base = (void*)((uintptr_t)base & ~(size - 1));

    u8 layer = get_layer_num(zone->size, size);
    size_t block_size = zone->size >> layer;

    if (crosses_block_boundary(prev_base, prev_size, block_size)) {
        size *= 2;
        base = (void*)((uintptr_t)base & ~(size - 1));
        layer--;
    }

    unsigned long block = (uintptr_t)base / block_size;
    if (!is_block_free(zone, layer, block))
        return -EADDRINUSE;

    return alloc_block(zone, layer, block);
}

static int _free_pages(struct zone* zone, void* addr, unsigned int order) {
    if (order >= zone->layers - 1u)
        return -E2BIG;

    u8 layer = zone->layers - order - 1;
    size_t block_size = 4096ul << order;

    unsigned long block = ((uintptr_t)addr - (uintptr_t)zone->base) / block_size;

    unsigned long flags;
    spinlock_lock_irq_save(&zone->lock, &flags);
    int err = free_block(zone, layer, block);
    spinlock_unlock_irq_restore(&zone->lock, &flags);
    return err;
}

static void* _alloc_pages(struct zone* zone, unsigned int order) {
    if (order >= zone->layers - 1u)
        return NULL;

    u8 layer = zone->layers - order - 1;
    size_t block_size = 4096ul << order;

    unsigned long block;
    int err;
    void* ret;

    unsigned long flags;
    spinlock_lock_irq_save(&zone->lock, &flags);

again:
    block = find_first_free(zone->map, layer);
    if (block == ULONG_MAX) {
        ret = NULL;
        goto out;
    }

    err = alloc_block(zone, layer, block);
    if (err) {
        ret = NULL;
        goto out;
    }

    ret = (u8*)zone->base + block * block_size;
    if ((u8*)ret + block_size >= (u8*)zone->base + zone->real_size) {
        ret = NULL;
        goto out;
    }

    /* 
     * If the region is not usable in the memory map, then this tries to allocate all reserved
     * memory regions within the memory block 4KiB at a time.
     */
    if (!is_region_usable(ret, block_size)) {
        free_block(zone, layer, block);
        for (size_t i = 0; i < block_size; i += 0x1000) {
            if (!is_region_usable((u8*)ret + i, 0x1000))
                memblock_reserve(zone, (u8*)ret + i, 0x1000);
        }
        goto again;
    }
out:
    spinlock_unlock_irq_restore(&zone->lock, &flags);
    return ret;
}

static struct zone* dma_zone;
static struct zone* dma32_zone;
static struct zone* normal_zone;

static struct zone* get_zone_from_gfp(unsigned int gfp_flags) {
    if (gfp_flags & GFP_PM_ZONE_DMA)
        return dma_zone;
    if (gfp_flags & GFP_PM_ZONE_DMA32)
        return dma32_zone;
    if (gfp_flags & GFP_PM_ZONE_NORMAL)
        return normal_zone;
    
    return NULL;
}

static struct zone* get_zone_from_addr(void* addr) {
    if (addr >= dma_zone->base && (u8*)addr < (u8*)dma_zone->base + dma_zone->size)
        return dma_zone;
    if (addr >= dma32_zone->base && (u8*)addr < (u8*)dma32_zone->base + dma32_zone->size)
        return dma32_zone;
    if (addr >= normal_zone->base && (u8*)addr < (u8*)normal_zone->base + normal_zone->size)
        return normal_zone;

    return NULL;
}

void* alloc_pages(unsigned int gfp_flags, unsigned int order) {
    struct zone* zone = get_zone_from_gfp(gfp_flags);
    if (!zone)
        return NULL;

    void* ret = _alloc_pages(zone, order);
    if (!ret) {
        switch (zone->type) {
        case GFP_PM_ZONE_NORMAL:
            zone = dma32_zone;
            ret = _alloc_pages(zone, order);
            if (ret)
                break;
        /* fall through */
        case GFP_PM_ZONE_DMA32:
            zone = dma_zone;
            ret = _alloc_pages(zone, order);
            if (ret)
                break;
        /* fall through */
        case GFP_PM_ZONE_DMA:
        default:
            return NULL;
        }
    }

    return ret;
}

void free_pages(void* addr, unsigned int order) {
    if (unlikely(addr < (void*)0x1000)) {
        printk(PL_ERR "free_pages tried to free the first page of physical memory!\n");
        return;
    }

    struct zone* zone = get_zone_from_addr(addr);
    if (!zone) {
        printk(PL_ERR "free_pages failed to get the zone type! (%p)\n", addr);
        return;
    }

    int err = _free_pages(zone, addr, order);
    if (err)
        printk(PL_ERR "_free_pages failed to free pages from the memory zone! err: %i\n", err);
}

static void memory_zone_init(struct zone* zone, void* base, 
        size_t size, size_t real_size, unsigned int type, u8 layers) {
    zone->base = base;
    zone->size = size;
    zone->real_size = real_size;
    zone->type = type;
    zone->layers = layers;
    zone->map = (u8*)(zone + 1);
    zone->lock = SPINLOCK_INITIALIZER;

    size_t map_size = (1ul << layers) / 8 + 1;
    memset(zone->map, 0, map_size);
}

static struct zone* alloc_dma_zone(size_t struct_size) {
    uintptr_t addr = 0x1000;
    while (1) {
        /* Allocate the first zone only within the DMA zone */
        if (addr >= 0x1000000 - struct_size) {
            addr = 0;
            break;
        }

        /* Found a usable first, region so use it */
        if (is_region_usable((void*)addr, struct_size))
            break;

        addr += 0x1000;
    }

    return addr ? hhdm_virtual((struct zone*)addr) : NULL;
}

static u8 calculate_layers(size_t zone_size) {
    u8 layers = 1;
    while (layers < 30) {
        size_t bs = zone_size >> (layers - 1);
        if (bs / 2 < 4096)
            break;
        layers++;
    }
    return layers;
}

static size_t dma_zone_init(uintptr_t base, size_t total_mem) {
    size_t zone_size = total_mem < 0x1000000 ? total_mem : 0x1000000;
    size_t zone_size_rounded = round_pow2_up(0x1000, zone_size);
    u8 layers = calculate_layers(zone_size_rounded);
    size_t map_size = (1ul << layers) / 8 + 1;

    dma_zone = alloc_dma_zone(sizeof(struct zone) + map_size);
    
    /* The bootloader is probably gonna fail before this even has a chance of happening */
    if (unlikely(!dma_zone))
        panic("Failed to initialize DMA memory zone");

    memory_zone_init(dma_zone, (void*)base, zone_size_rounded, zone_size, GFP_PM_ZONE_DMA, layers);
    memblock_reserve(dma_zone, hhdm_physical(dma_zone), sizeof(struct zone) + map_size);
    return zone_size;
}

static size_t dma32_zone_init(uintptr_t base, size_t total_mem) {
    size_t zone_size = total_mem < 0x100000000 ? total_mem : 0x100000000;
    size_t zone_size_rounded = round_pow2_up(4096, zone_size);
    u8 layers = calculate_layers(zone_size_rounded);

    /* Unlikely to happen since most systems have more than 16MiB of ram */
    if (unlikely(layers == 1)) {
        dma32_zone = dma_zone;
        normal_zone = dma_zone;
        return 0;
    }

    size_t map_size = (1ul << layers) / 8 + 1;
    dma32_zone = _alloc_pages(dma_zone, get_order(sizeof(struct zone) + map_size));
    if (!dma32_zone)
        panic("Failed to initialize dma32 zone");
    dma32_zone = hhdm_virtual(dma32_zone);

    memory_zone_init(dma32_zone, (void*)base, zone_size_rounded, zone_size, GFP_PM_ZONE_DMA32, layers);
    return zone_size;
}

static void normal_zone_init(uintptr_t base, size_t total_mem) {
    size_t zone_size_rounded = round_pow2_up(4096, total_mem);
    u8 layers = calculate_layers(zone_size_rounded);

    if (layers == 1) {
        normal_zone = dma32_zone;
        return;
    }

    size_t map_size = (1ul << layers) / 8 + 1;
    normal_zone = _alloc_pages(dma32_zone, get_order(sizeof(struct zone) + map_size));
    if (!normal_zone)
        panic("Failed to initialize the normal memory zone");
    normal_zone = hhdm_virtual(normal_zone);

    memory_zone_init(normal_zone, (void*)base, zone_size_rounded, total_mem, GFP_PM_ZONE_NORMAL, layers);
}

void memory_zones_init(void) {
    if (unlikely(!g_limine_mmap_request.response))
        panic("Limine memory map response not available!");

    /* Getting the very last usable memory address tells the kernel how much memory is available */
    size_t total_mem = (uintptr_t)get_last_usable();
    uintptr_t base = 0;
    
    size_t zone_size = dma_zone_init(base, total_mem);

    /* 
     * Reserve the first page of memory so NULL can be returned when no memory is available.
     * The limine memory map also never marks this region as usable
     */
    memblock_reserve(dma_zone, (void*)base, 0x1000);
    base += zone_size;
    total_mem -= zone_size;

    zone_size = dma32_zone_init(base, total_mem);
    if (unlikely(zone_size == 0)) {
        printk("Initialized physical memory zones: DMA\n");
        return;
    }

    base += zone_size;
    total_mem -= zone_size;

    normal_zone_init(base, total_mem);
    if (dma32_zone == normal_zone)
        printk("Initialized physical memory zones: DMA, DMA32\n");
    else
        printk("Initialized physical memory zones: DMA, DMA32, Normal\n");
}
