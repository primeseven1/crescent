#pragma once

typedef enum {
    GFP_PM_ZONE_DMA = (1 << 0),
    GFP_PM_ZONE_DMA32 = (1 << 1),
    GFP_PM_ZONE_NORMAL = (1 << 2),
    GFP_VM_KERNEL = (1 << 3),
    GFP_VM_EXEC = (1 << 4),
    GFP_VM_LARGE_PAGE = (1 << 5),
    GFP_ZERO = (1 << 6)
} gfp_t;

#define GFP_PM_FLAGS_MASK (GFP_PM_ZONE_DMA | GFP_PM_ZONE_DMA32 | GFP_PM_ZONE_NORMAL)
#define GFP_VM_FLAGS_MASK (GFP_VM_KERNEL | GFP_VM_EXEC | GFP_VM_LARGE_PAGE)
#define GFP_MISC_FLAGS_MASK (GFP_ZERO)
