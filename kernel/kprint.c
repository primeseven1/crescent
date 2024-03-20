#include <crescent/cpu/ioports.h>
#include <crescent/conversions.h>
#include <crescent/string.h>
#include <crescent/types.h>

static void kprint_format_llong(long long val, int base, void (*cb)(const char*))
{
    char str_val[sizeof(long long) * 8 + 1];
    int err = lltostr(val, str_val, base, sizeof(str_val));
    if (!err)
        cb(str_val);
    else
        cb("(invalid)");
}

static void kprint_format_ullong(unsigned long long val, int base, void (*cb)(const char*))
{
    char str_val[sizeof(long long) * 8 + 1];
    int err = ulltostr(val, str_val, base, sizeof(str_val));
    if (!err)
        cb(str_val);
    else
        cb("(invalid)");
}

static void kprint_format_double(double val, int afterpoint, void (*cb)(const char*))
{
    char str_val[sizeof(double) * 8 + 1];
    int err = dtostr(val, str_val, afterpoint, sizeof(str_val));
    if (!err)
        cb(str_val);
    else
        cb("(invalid)");
}

/* Returns the amount of characters it had to parse */
static unsigned int handle_multichar_fmt_l(const char* rest, va_list va, void (*cb)(const char*))
{
    switch (*++rest) {
    case 'd':
    case 'i':
        kprint_format_llong(va_arg(va, long), 10, cb);
        return 1;
    case 'u':
        kprint_format_ullong(va_arg(va, unsigned long), 10, cb);
        return 1;
    case 'l':
        rest++;
        if (*rest == 'i' || *rest == 'd')
            kprint_format_llong(va_arg(va, long long), 10, cb);
        else if (*rest == 'u')
            kprint_format_ullong(va_arg(va, unsigned long long), 10, cb);
        else if (*rest == 'x')
            kprint_format_ullong(va_arg(va, unsigned long long), 16, cb);

        return 2;
    case 'x':
        kprint_format_ullong(va_arg(va, unsigned long), 16, cb);
        return 1;
    case 'f':
        kprint_format_double(va_arg(va, double), 5, cb);
        return 1;
    default:
        break;
    }

    return 0;
}

#ifdef CONFIG_DEBUG

void _kprint_e9_noformat(const char* str)
{
    while (*str)
        outportb(0xE9, *str++);
}

void _vkprint_e9(const char* fmt, va_list va)
{
    while (*fmt) {
        if (*fmt != '%') {
            outportb(0xE9, *fmt++);
            continue;
        }

        switch (*++fmt) {
        case '%':
            outportb(0xE9, '%');
            break;
        case 'i':
        case 'd':
            kprint_format_llong(va_arg(va, int), 10, _kprint_e9_noformat);
            break;
        case 'u':
            kprint_format_ullong(va_arg(va, unsigned int), 10, _kprint_e9_noformat);
            break;
        case 'x':
            kprint_format_ullong(va_arg(va, unsigned int), 16, _kprint_e9_noformat);
            break;
        case 'f':
            kprint_format_double((float)va_arg(va, double), 5, _kprint_e9_noformat);
            break;
        case 'l': {
            unsigned int add = handle_multichar_fmt_l(fmt, va, _kprint_e9_noformat);
            fmt += add;
            break;
        }
        case 'p':
            _kprint_e9_noformat("0x");
            kprint_format_ullong((unsigned long long)va_arg(va, void*), 16, _kprint_e9_noformat);
            break;
        case 's':
            _kprint_e9_noformat(va_arg(va, const char*));
            break;
        default:
            break;
        }

        fmt++;
    } 
}

void _kprint_e9(const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    _vkprint_e9(fmt, va);
    va_end(va);
}

#endif /* CONFIG_DEBUG */
