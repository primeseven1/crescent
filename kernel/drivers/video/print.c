#include <crescent/drivers/video/video.h>
#include <crescent/core/limine.h>
#include <crescent/core/locking.h>
#include <crescent/common.h>
#include <crescent/lib/string.h>
#include <crescent/lib/fonts.h>
#include "video.h"

static unsigned int max_row_index;
static unsigned int max_column_index;

static struct limine_framebuffer* framebuffer = NULL;
static const struct font_data* font = NULL;

static void put_char(unsigned char c, u32 x, u32 y, u8 red, u8 green, u8 blue) {
    x *= font->width;
    y *= font->height;

    if (x > framebuffer->width || y > framebuffer->height)
        return;

    for (int row = 0; row < font->height; row++) {
        /* printing with col = 0; col < 8; col++ will result in the text being mirrored */
        for (int col = font->width - 1; col >= 0; col--) {
            const u8* font_data = font->font_data + c * font->height;
            if (font_data[row] & (1 << col)) {
                _driver_video_put_pixel(framebuffer, x + font->width - col - 1, 
                        y + row, red, green, blue);
            }
        }
    }
}

static void scroll_text(void) {
    /* Start out by copying every pixel from the second row, and moving it to the top row */
    size_t index = (font->height * framebuffer->pitch);
    void* addr = (u8*)framebuffer->addr + index;
    size_t count = framebuffer->width * framebuffer->height * (framebuffer->bpp / 8) -
                   (framebuffer->width * (framebuffer->bpp / 8) * font->height);
    memmove(framebuffer->addr, addr, count);

    /* Now just clear the last row */
    index = ((framebuffer->height - font->height) * framebuffer->pitch);
    addr = (u8*)framebuffer->addr + index;
    count = framebuffer->width * (framebuffer->bpp / 8) * font->height;
    memset(addr, 0, count);
}

static unsigned int row_index = 0;
static unsigned int column_index = 0;

static bool handle_escape(char c) {
    switch (c) {
    case '\n':
        row_index = 0;
        column_index++;
        if (column_index > max_column_index) {
            scroll_text();
            column_index = max_column_index;
        }
        return true;
    case '\b':
        if (row_index)
            row_index--;
        else if (row_index == 0 && column_index)
            row_index = max_row_index;
        return true;
    case '\a':
        return true;
    case '\r':
        row_index = 0;
        return true;
    case '\v': {
        column_index++;
        if (column_index > max_column_index) {
            scroll_text();
            column_index = max_column_index;
        }
        return true;
    }
    case '\t': {
        const unsigned int tab_width = 8;
        unsigned int next_tab_stop = (row_index / tab_width + 1) * tab_width;

        row_index = next_tab_stop;
        if (row_index > max_row_index) {
            row_index = 0;
            column_index++;

            if (column_index > max_column_index) {
                scroll_text();
                column_index = max_column_index;
            }
        }

        return true;
    }
    }

    return false;
}

static void print_string(const char* str, int len, u8 red, u8 green, u8 blue) {
    while (len--) {
        if (*str < 32) {
            bool handled = handle_escape(*str);
            if (handled) {
                str++;
                continue;
            }
        }

        put_char((unsigned char)*str, row_index, column_index, red, green, blue);

        str++;
        row_index++;

        if (row_index > max_row_index) {
            row_index = 0;
            column_index++;

            if (column_index > max_column_index) {
                scroll_text();
                column_index = max_column_index;
            }
        }
    }
}

void driver_video_printk(const char* str, int len) {
    print_string(str, len, 0xFF, 0xFF, 0xFF);
}

void _driver_video_print_init(struct limine_framebuffer* fb) {
    font = &g_font_8x16;
    max_row_index = fb->width / font->width - 1;
    max_column_index = fb->height / font->height - 1;
    framebuffer = fb;
}
