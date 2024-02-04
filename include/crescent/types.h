#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <limits.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define FIELD_SIZEOF(t, f) (sizeof(((t*)0)->f))
#define NULL ((void*)0)

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef i32 ssize_t;
typedef u32 size_t;

typedef enum {
    false = 0,
    true
} __attribute__((packed)) bool;
