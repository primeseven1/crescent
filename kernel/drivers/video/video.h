#pragma once

#include <crescent/core/limine.h>

void _driver_video_put_pixel(struct limine_framebuffer* fb_dev, 
        u32 x, u32 y, u8 red, u8 green, u8 blue);
void _driver_video_clear_screen(struct limine_framebuffer* fb_dev, u8 red, u8 green, u8 blue);
void _driver_video_print_init(struct limine_framebuffer* fb);
