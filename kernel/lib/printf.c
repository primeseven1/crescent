#include <crescent/asm/errno.h>
#include <crescent/lib/string.h>
#include <crescent/lib/kxtostr.h>
#include <crescent/lib/printf.h>

static int printf_char(char* dest, char c, size_t dsize) {
    char s[2] = { c, '\0' };
    size_t char_count = strlcat(dest, s, dsize);

    if (strlen(dest) != char_count)
        return -EOVERFLOW;

    return 0;
}

static int printf_string(char* dest, const char* s, size_t dsize) {
    size_t char_count;
    if (s)
        char_count = strlcat(dest, s, dsize);
    else
        char_count = strlcat(dest, "(null)", dsize);

    if (strlen(dest) != char_count)
        return -EOVERFLOW;

    return 0;
}

static inline void hex_to_upper(char* dest) {
    while (*dest) {
        if (*dest >= 'a' && *dest <= 'f')
            *dest -= 0x20;
        dest++;
    }
}

static int printf_int(char* dest, long long x, int base, size_t dsize, bool upper) {
    size_t len = strlen(dest);
    if (len >= dsize)
        return -EOVERFLOW;

    dest += len;
    dsize -= len;

    int err = klltostr(dest, x, base, dsize);
    if (!err && upper)
        hex_to_upper(dest);
    return err;
}

static int printf_uint(char* dest, unsigned long long x, int base, size_t dsize, bool upper) {
    size_t len = strlen(dest);
    if (len >= dsize)
        return -EOVERFLOW;

    dest += len;
    dsize -= len;

    int err = kulltostr(dest, x, base, dsize);
    if (!err && upper)
        hex_to_upper(dest);
    return err;
}

int vsnprintf(char* buf, size_t bufsize, const char* fmt, va_list va) {
    *buf = '\0';

    while (*fmt) {
        if (*fmt != '%') {
            char c[2] = { *fmt, '\0' };
            strlcat(buf, c, bufsize);
            fmt++;
            continue;
        }

        int err;
        bool hex_upper = false;

        switch (*++fmt) {
        case 'c':
            err = printf_char(buf, (char)va_arg(va, int), bufsize);
            break;
        case 'd':
        case 'i':
            err = printf_int(buf, va_arg(va, int), 10, bufsize, false);
            break;
        case 'u':
            err = printf_uint(buf, va_arg(va, unsigned int), 10, bufsize, false);
            break;
        case 'X':
            hex_upper = true;
            /* fall through */
        case 'x':
            err = printf_uint(buf, va_arg(va, unsigned int), 16, bufsize, hex_upper);
            break;
        case 'o':
            err = printf_uint(buf, va_arg(va, unsigned int), 8, bufsize, false);
            break;
        case 'p':
            err = printf_string(buf, "0x", bufsize);
            if (err)
                return err;
            err = printf_uint(buf, (unsigned long long)va_arg(va, void*), 16, bufsize, false);
            break;
        case 's':
            err = printf_string(buf, va_arg(va, const char*), bufsize);
            break;
        case 'h':
            switch (*++fmt) {
            case 'd':
            case 'i':
                err = printf_int(buf, (short)va_arg(va, int), 10, bufsize, false);
                break;
            case 'u':
                err = printf_uint(buf, (unsigned short)va_arg(va, unsigned int), 10, bufsize, false);
                break;
            case 'X':
                hex_upper = true;
                /* fall through */
            case 'x':
                err = printf_uint(buf, (unsigned short)va_arg(va, unsigned int), 16, bufsize, hex_upper);
                break;
            case 'o':
                err = printf_uint(buf, (unsigned short)va_arg(va, unsigned int), 8, bufsize, false);
                break;
            default:
                err = 0;
                break;
            }
            break;
        case 'z':
            switch (*++fmt) {
            case 'd':
            case 'i':
                err = printf_int(buf, va_arg(va, ssize_t), 10, bufsize, false);
                break;
            case 'u':
                err = printf_uint(buf, va_arg(va, size_t), 10, bufsize, false);
                break;
            case 'X':
                hex_upper = true;
                /* fall through */
            case 'x':
                err = printf_uint(buf, va_arg(va, size_t), 16, bufsize, hex_upper);
                break;
            case 'o':
                err = printf_uint(buf, va_arg(va, size_t), 8, bufsize, false);
                break;
            default:
                err = 0;
                break;
            }
            break;
        case 'l':
            switch (*++fmt) {
            case 'd':
            case 'i':
                err = printf_int(buf, va_arg(va, long), 10, bufsize, false);
                break;
            case 'u':
                err = printf_uint(buf, va_arg(va, unsigned long), 10, bufsize, false);
                break;
            case 'X':
                hex_upper = true;
                /* fall through */
            case 'x':
                err = printf_uint(buf, va_arg(va, unsigned long), 16, bufsize, hex_upper);
                break;
            case 'o':
                err = printf_uint(buf, va_arg(va, unsigned long), 8, bufsize, false);
                break;
            case 'l':
                switch (*++fmt) {
                case 'd':
                case 'i':
                    err = printf_int(buf, va_arg(va, long long), 10, bufsize, false);
                    break;
                case 'u':
                    err = printf_uint(buf, va_arg(va, unsigned long long), 10, bufsize, false);
                    break;
                case 'X':
                    hex_upper = true;
                    /* fall through */
                case 'x':
                    err = printf_uint(buf, va_arg(va, unsigned long long), 16, bufsize, hex_upper);
                    break;
                case 'o':
                    err = printf_uint(buf, va_arg(va, unsigned long long), 8, bufsize, false);
                    break;
                default:
                    err = 0;
                    break;
                }
                break;
            default:
                err = 0;
                break;
            }
            break;
        default:
            err = 0;
            break;
        }

        if (err)
            return err;

        fmt++;
    }

    return strlen(buf);
}

int snprintf(char* buf, size_t bufsize, const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    int ret = vsnprintf(buf, bufsize, fmt, va);
    va_end(va);
    return ret;
}
