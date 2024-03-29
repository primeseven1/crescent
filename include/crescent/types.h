#pragma once

#include <stdint.h>
#include <stdarg.h> /* IWYU pragma: keep */
#include <limits.h> /* IWYU pragma: keep */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define FIELD_SIZEOF(t, f) (sizeof(((t*)0)->f))
#define OFFSETOF(t, f) ((size_t)&(((t*)0)->f))
#define NULL ((void*)0)

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef i64 ssize_t;
typedef u64 size_t;

typedef enum {
    false = 0,
    true
} __attribute__((packed)) bool;
