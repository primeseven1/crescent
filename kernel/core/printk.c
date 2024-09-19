#include <crescent/common.h>
#include <crescent/core/printk.h>
#include <crescent/core/locking.h>
#include <crescent/core/debug/e9.h>
#include <crescent/lib/kxtostr.h>
#include <crescent/lib/string.h>

static int printf_char(char* dest, char c, size_t dsize) {
    char s[2] = { c, '\0' };
    size_t char_count = strlcat(dest, s, dsize);

    if (strlen(dest) != char_count)
        return -EOVERFLOW;

    return 0;
}

static int printf_string(char* dest, const char* s, size_t dsize) {
    size_t char_count;
    if (dest)
        char_count = strlcat(dest, s, dsize);
    else
        char_count = strlcat(dest, "(null)", dsize);

    if (strlen(dest) != char_count)
        return -EOVERFLOW;

    return 0;
}

static int printf_int(char* dest, long long x, int base, size_t dsize) {
    size_t len = strlen(dest);
    if (len >= dsize)
        return -EOVERFLOW;

    dest += len;
    dsize -= len;
    return klltostr(dest, x, base, dsize);
}

static int printf_uint(char* dest, unsigned long long x, int base, size_t dsize) {
    size_t len = strlen(dest);
    if (len >= dsize)
        return -EOVERFLOW;

    dest += len;
    dsize -= len;
    return kulltostr(dest, x, base, dsize);
}

static int printf_dbl(char* dest, double x, int afterpoint, size_t dsize) {
    size_t len = strlen(dest);
    if (len >= dsize)
        return -EOVERFLOW;

    dest += len;
    dsize -= len;
    return kdbltostr(dest, x, afterpoint, dsize);
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

        switch (*++fmt) {
        case 'c':
            err = printf_char(buf, (char)va_arg(va, int), bufsize);
            break;
        case 'd':
        case 'i':
            err = printf_int(buf, va_arg(va, int), 10, bufsize);
            break;
        case 'u':
            err = printf_uint(buf, va_arg(va, unsigned int), 10, bufsize);
            break;
        case 'x':
            err = printf_uint(buf, va_arg(va, unsigned int), 16, bufsize);
            break;
        case 'p':
            err = printf_string(buf, "0x", bufsize);
            if (err)
                return err;
            err = printf_uint(buf, (unsigned long long)va_arg(va, void*), 16, bufsize);
            break;
        case 'f':
            err = printf_dbl(buf, (float)va_arg(va, double), 5, bufsize);
            break;
        case 's':
            err = printf_string(buf, va_arg(va, const char*), bufsize);
            break;
        case 'h':
            switch (*++fmt) {
            case 'd':
            case 'i':
                err = printf_int(buf, (short)va_arg(va, int), 10, bufsize);
                break;
            case 'u':
                err = printf_uint(buf, (unsigned short)va_arg(va, unsigned int), 10, bufsize);
                break;
            case 'x':
                err = printf_uint(buf, (unsigned short)va_arg(va, unsigned int), 16, bufsize);
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
                err = printf_int(buf, va_arg(va, long), 10, bufsize);
                break;
            case 'u':
                err = printf_uint(buf, va_arg(va, unsigned long), 10, bufsize);
                break;
            case 'x':
                err = printf_uint(buf, va_arg(va, unsigned long), 16, bufsize);
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
                err = printf_int(buf, va_arg(va, long), 10, bufsize);
                break;
            case 'u':
                err = printf_uint(buf, va_arg(va, unsigned long), 10, bufsize);
                break;
            case 'x':
                err = printf_uint(buf, va_arg(va, unsigned long), 16, bufsize);
                break;
            case 'f':
                err = printf_dbl(buf, va_arg(va, double), 10, bufsize);
                break;
            case 'l':
                switch (*++fmt) {
                case 'd':
                case 'i':
                    err = printf_int(buf, va_arg(va, long long), 10, bufsize);
                    break;
                case 'u':
                    err = printf_uint(buf, va_arg(va, unsigned long long), 10, bufsize);
                    break;
                case 'x':
                    err = printf_uint(buf, va_arg(va, unsigned long long), 16, bufsize);
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

static char printk_buf[2048];
static const char* prefixes[] = {
    [PL_NONE_N] = "",
    [PL_EMERG_N] = "[EMERG] ",
    [PL_CRIT_N] = "[CRIT] ",
    [PL_ERR_N] = "[ERR] ",
    [PL_WARN_N] = "[WARN] ",
    [PL_INFO_N] = "[INFO] "
};
static spinlock_t lock = SPINLOCK_INITIALIZER;

static void 
default_printk_hook(__attribute__((unused)) const char* str, __attribute__((unused)) int len) {

}

static void (*printk_hook)(const char* str, int len) = default_printk_hook;

void printk_set_hook(void (*hook)(const char*, int)) {
    printk_hook = hook;
}

#ifdef CONFIG_DEBUG
static int printk_level = PL_INFO_N;
#else
static int printk_level = PL_ERR_N;
#endif

int set_printk_level(int level) {
    if (level > PL_DEFAULT_N)
        return -E2BIG;

    printk_level = level;
    return 0;
}

int vprintk(const char* fmt, va_list va) {
    int level;
    if (fmt[0] == PL_BEGIN_N) {
        if (__builtin_strlen(fmt) <= 2)
            return -EOVERFLOW;
        if (fmt[1] > PL_DEFAULT_N)
            level = PL_DEFAULT_N;
        else
            level = fmt[1];
        fmt += 2;
    } else {
        level = PL_DEFAULT_N;
    }

    const char* prefix = prefixes[level];
    int prefix_len = strlen(prefix);

    unsigned long flags;
    spinlock_lock_irq_save(&lock, &flags);
    int len = vsnprintf(printk_buf, sizeof(printk_buf), fmt, va);
    if (len > 0 && level <= printk_level) {
        printk_hook(prefix, prefix_len);
        printk_hook(printk_buf, len);
    }

#ifdef CONFIG_E9_ENABLE
    debug_e9_write_str(prefix, prefix_len);
    debug_e9_write_str(printk_buf, len);
#endif /* CONFIG_E9_ENABLE */

    spinlock_unlock_irq_restore(&lock, &flags);
    return len;
}

int printk(const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    int ret = vprintk(fmt, va);
    va_end(va);
    return ret;
}
