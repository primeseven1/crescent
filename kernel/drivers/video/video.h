#pragma once

#include <crescent/core/limine.h>

/**
 * @brief Put a pixel on a linear framebuffer
 *
 * @param fb_dev The framebuffer device to use
 * @param x,y The coordinates of the pixel, starting from the top left
 * @param red,green,blue The RGB values to use for the pixel
 */
void _driver_video_put_pixel(struct limine_framebuffer* fb_dev, 
        u32 x, u32 y, u8 red, u8 green, u8 blue);

/**
 * @brief Clear the framebuffer with one color
 *
 * @param fb_dev The framebuffer device to use
 * @param red,green,blue The RGB values to use for the entire screen
 */
void _driver_video_clear_screen(struct limine_framebuffer* fb_dev, u8 red, u8 green, u8 blue);

/**
 * @brief Initialize a framebuffer for printing strings
 * @param fb The framebuffer to use for printing
 */
void _driver_video_print_init(struct limine_framebuffer* fb);
