#pragma once

enum gfp_flags {
    GFP_PM_ZONE_DMA = (1 << 0),
    GFP_PM_ZONE_DMA32 = (1 << 1),
    GFP_PM_ZONE_NORMAL = (1 << 2),
    GFP_PM_CONTIGUOUS = (1 << 3),
    GFP_VM_KERNEL = (1 << 4),
    GFP_VM_EXEC = (1 << 5),
    GFP_VM_LARGE_PAGE = (1 << 6)
};
