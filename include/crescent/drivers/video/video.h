#pragma once

#include <crescent/asm/errno.h>

/**
 * @brief Initialize the basic video driver
 */
int driver_video_init(void);

/**
 * @brief A printk hook for printing text to the string
 *
 * @param str The string to print
 * @param len The length of the string
 */
void driver_video_printk(const char* str, int len);
