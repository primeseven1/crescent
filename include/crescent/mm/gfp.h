#pragma once

enum gfp_flags {
    GFP_ZONE_DMA = (1 << 0),
    GFP_ZONE_DMA32 = (1 << 1),
    GFP_ZONE_NORMAL = (1 << 2),
    GFP_VM_SHARED = (1 << 3),
    GFP_VM_HUGE = (1 << 4),
    GFP_VM_USER = (1 << 5)
};
