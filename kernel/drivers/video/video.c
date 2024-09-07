#include <crescent/common.h>
#include <crescent/core/limine.h>
#include <crescent/lib/string.h>
#include <crescent/drivers/video/video.h>
#include "video.h"

void _driver_video_put_pixel(struct limine_framebuffer* fb_dev, 
        u32 x, u32 y, u8 red, u8 green, u8 blue) {
    u32 color = (red << fb_dev->red_mask_shift) | 
                (green << fb_dev->green_mask_shift) | 
                (blue << fb_dev->blue_mask_shift);
    u64 index = x * (fb_dev->bpp / 8) + (y * fb_dev->pitch);

    /* Using a union to avoid casting I guess. Also, this is better than a void pointer */
    union {
        u32* addr32;
        u8* addr8;
    } addr = {
        .addr8 = (u8*)fb_dev->addr + index
    };

    /* Hopefully this framebuffer has 32 bit bpp */
    if (fb_dev->bpp == 32) {
        *addr.addr32 = color;
    } else if (fb_dev->bpp == 24) {
        addr.addr8[0] = color & 0xFF;
        addr.addr8[1] = (color >> 8) & 0xFF;
        addr.addr8[2] = (color >> 16 & 0xFF);
    }
}

void _driver_video_clear_screen(struct limine_framebuffer* fb_dev, u8 red, u8 green, u8 blue) {
    u32 color = (red << fb_dev->red_mask_shift) | 
                (green << fb_dev->green_mask_shift) | 
                (blue << fb_dev->blue_mask_shift);

    /* If this framebuffer doesn't have 32 bit bpp, that just kind of sucks */
    if (fb_dev->bpp == 32) {
        u32* addr = fb_dev->addr;
        size_t count = (fb_dev->width * fb_dev->height * (fb_dev->bpp / 8)) / 4;
        while (count--)
            *addr++ = color;
    } else if (fb_dev->bpp == 24) {
        u8* addr = fb_dev->addr;
        size_t count = (fb_dev->width * fb_dev->height * (fb_dev->bpp / 8)) / 3;
        while (count--) {
            addr[0] = color & 0xFF;
            addr[1] = (color >> 8) & 0xFF;
            addr[2] = (color >> 16) & 0xFF;
            addr += 3;
        }
    }
}

int driver_video_init(void) {
    struct limine_framebuffer_response* fb_response = g_limine_framebuffer_request.response;
    if (!fb_response)
        return -ENOPROTOOPT;
    if (!fb_response->framebuffer_count)
        return -ENODEV;

    /* Check for a framebuffer that the driver knows how to properly use */
    struct limine_framebuffer* fb_dev = NULL;
    for (u64 i = 0; i < fb_response->framebuffer_count; i++) {
        struct limine_framebuffer* dev = fb_response->framebuffers[i];
        if (dev->memory_model != LIMINE_FRAMEBUFFER_RGB || 
                (dev->bpp != 24 && dev->bpp != 32) || dev->red_mask_size != 8 ||
                dev->green_mask_size != 8 || dev->blue_mask_size != 8)
            continue;

        fb_dev = dev;
        break;
    }

    /* No supported framebuffer found */
    if (!fb_dev)
        return -ENOSYS;
    
    _driver_video_print_init(fb_dev);
    _driver_video_clear_screen(fb_dev, 0, 0, 0);
    return 0;
}
