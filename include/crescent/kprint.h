#pragma once

#include <crescent/types.h>

void kprint_format_llong(long long val, int base, void (*cb)(const char*));
void kprint_format_ullong(unsigned long long val, int base, void (*cb)(const char*));
void kprint_format_double(double val, int afterpoint, void (*cb)(const char*));

#ifdef CONFIG_DEBUG

void _kprint_e9_noformat(const char* str);
void _vkprint_e9(const char* fmt, va_list va);
__attribute__((format(printf, 1, 2)))
void _kprint_e9(const char* fmt, ...);

#define DBG_E9_KPRINT_NOFMT(str) _kprint_e9_noformat(str)
#define DBG_E9_VKPRINT(fmt, va) _vkprint_e9(fmt, va)
#define DBG_E9_KPRINT(fmt, ...) _kprint_e9(fmt, __VA_ARGS__)

#else /* CONFIG_DEBUG */

#define DBG_E9_KPRINT_NOFMT(str)
#define DBG_E9_VKPRINT(fmt, va)
#define DBG_E9_KPRINT(fmt, ...)

#endif /* CONFIG_DEBUG */
