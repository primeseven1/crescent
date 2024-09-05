#pragma once

#include <crescent/types.h>

/**
 * @brief Write an 8-bit value to an I/O port
 *
 * @param port The I/O port to write to
 * @param val The 8-bit value to write
 */
static inline void outb(u16 port, u8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * @brief Write a 16-bit value to an I/O port
 *
 * @param port The I/O port to write to
 * @param val The 16-bit value to write
 */
static inline void outw(u16 port, u16 val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * @brief Write a 32-bit value to an I/O port
 *
 * @param port The I/O port to write to
 * @param val The 32-bit value to write
 */
static inline void outl(u16 port, u32 val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * @brief Read an 8-bit value from and I/O port
 *
 * @param port The I/O port to read from
 * @return The 8-bit value read from the port
 */
static inline u8 inb(u16 port) {
    u8 ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/**
 * @brief Read a 16-bit value from and I/O port
 *
 * @param port The I/O port to read from
 * @return The 16-bit value read from the port
 */
static inline u16 inw(u16 port) {
    u16 ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}


/**
 * @brief Read a 32-bit value from and I/O port
 *
 * @param port The I/O port to read from
 * @return The 8-bit value read from the port
 */
static inline u32 inl(u16 port) {
    u32 ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/**
 * @brief Wait for an I/O operation to complete
 *
 * This function writes to an unused I/O port (0x80) to introduce a small delay,
 * allowing an I/O operation to complete.
 */
static inline void pmio_wait(void) {
    outb(0x80, 0);
}
