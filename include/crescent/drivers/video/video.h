#pragma once

#include <crescent/asm/errno.h>

int driver_video_init(void);
void driver_video_printk(const char* str, int len);
