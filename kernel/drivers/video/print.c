#include <crescent/drivers/video/video.h>
#include <crescent/common.h>
#include <crescent/core/limine.h>
#include <crescent/core/locking.h>
#include <crescent/lib/string.h>
#include <crescent/lib/fonts.h>
#include <crescent/lib/kstrtox.h>
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

struct color_info {
    u8 red;
    u8 green;
    u8 blue;
};

static struct color_info color_info[8] = {
    { .red = 0x00, .green = 0x00, .blue = 0x00 }, /* black */
    { .red = 0x80, .green = 0x00, .blue = 0x00 }, /* red */
    { .red = 0x00, .green = 0x80, .blue = 0x00 }, /* green */
    { .red = 0x80, .green = 0x80, .blue = 0x00 }, /* yellow */
    { .red = 0x00, .green = 0x00, .blue = 0x80 }, /* blue */
    { .red = 0x80, .green = 0x00, .blue = 0x80 }, /* magenta */
    { .red = 0x00, .green = 0x80, .blue = 0x80 }, /* cyan */
    { .red = 0xFF, .green = 0xFF, .blue = 0xFF } /* white */
};
static struct color_info default_color = {
    .red = 0xcc, .green = 0xcc, .blue = 0xcc
};

static struct color_info* current = &default_color;

/* Returns the amount of characters that should be added to skip the escape sequence, returns -1 on error */
static ssize_t change_ansi_color(const char* s) {
    size_t len = 0;
    while (s[len] != 'm') {
        if (s[len] == '\0')
            return -1;
        len++;
    }

    unsigned long color;
    int err = kstrtoul(s, len, 10, &color);
    if (err)
        return -1;
    if (color == 0) {
        current = &default_color;
        return len + 1;
    }

    /* foreground colors start at 30 */
    color -= 30;
    if (color >= ARRAY_SIZE(color_info))
        return -1;

    current = &color_info[color];
    return len + 1;
}

void driver_video_printk(const char* str, int len) {
    while (len) {
        if (str[0] == '\033' && str[1] == '[') {
            ssize_t add = change_ansi_color(str + 2);
            if (add == -1) {
                str++;
                len--;
                continue;
            }
            str += (size_t)add + 2;
            len -= add + 2;
            continue;
        } else if (*str < 32) {
            bool handled = handle_escape(*str);
            if (handled) {
                str++;
                len--;
                continue;
            }
        }

        put_char((unsigned char)*str, row_index, column_index, 
                current->red, current->green, current->blue);

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

        len--;
    }
}

void _driver_video_print_init(struct limine_framebuffer* fb) {
    font = &g_font_8x16;
    max_row_index = fb->width / font->width - 1;
    max_column_index = fb->height / font->height - 1;
    framebuffer = fb;
}
