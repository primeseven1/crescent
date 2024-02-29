#pragma once

#include <crescent/types.h>

static inline void outportb(u16 port, u8 val)
{
    asm volatile("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline void outportw(u16 port, u16 val)
{
    asm volatile("outw %w0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline void outportl(u16 port, u32 val)
{
    asm volatile("outl %l0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline u8 inportb(u16 port)
{
    u8 ret;
    asm volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static inline u16 inportw(u16 port)
{
    u16 ret;
    asm volatile("inw %w1, %w0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static inline u32 inportl(u16 port)
{
    u32 ret;
    asm volatile("inl %w1, %l0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static inline void io_wait(void)
{
    outportb(0x80, 0);
}
