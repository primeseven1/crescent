#pragma once

/* LP64: char: 8 bits, short: 16 bits, int: 32 bits, long: 64 bits, pointer: 64 bits */
#ifndef __LP64__
#error "LP64 64 bit data model not used! Cannot compile."
#endif /* __LP64__ */

#include <crescent/limits.h>

typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef long i64;
typedef unsigned long u64;

typedef long ssize_t;
typedef unsigned long size_t;

typedef long intptr_t;
typedef unsigned long uintptr_t;

typedef __builtin_va_list va_list;

typedef _Bool bool;

#define I8_MIN CHAR_MIN
#define I8_MAX CHAR_MAX
#define U8_MAX UCHAR_MAX
#define I16_MIN SHRT_MIN
#define I16_MAX SHRT_MAX
#define U16_MAX USHRT_MAX
#define I32_MIN INT_MIN
#define I32_MAX INT_MAX
#define U32_MAX UINT_MAX
#define I64_MIN LONG_MIN
#define I64_MAX LONG_MAX
#define U64_MAX ULONG_MAX

#define SSIZE_MIN LONG_MIN
#define SSIZE_MAX LONG_MAX
#define SIZE_MAX ULONG_MAX

#define INTPTR_MIN LONG_MIN
#define INTPTR_MAX LONG_MAX
#define UINTPTR_MAX ULONG_MAX

#define va_start(v, p) __builtin_va_start(v, p)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, t) __builtin_va_arg(v, t)
#define va_copy(d, s) __builtin_va_copy(d, s)

#define true 1
#define false 0
