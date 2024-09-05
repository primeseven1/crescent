#pragma once

#include <crescent/types.h>

/**
 * @brief Formats and stores a va_list into a buffer
 *
 * @param buf The buffer where the string will be stored
 * @param bufsize The size of the buffer
 * @param fmt The format string
 * @param va The variable argument list
 *
 * @return The number of characters written, negative if an error happened
 */
int vsnprintf(char* buf, size_t bufsize, const char* fmt, va_list va);

/**
 * @brief Set a hook for the printk function
 *
 * This function will set the hook for printk, which printk will call after
 * it's done handling the formatted string
 *
 * @param hook The function printk should use
 */
void printk_set_hook(void (*hook)(const char*, int));

/**
 * @brief Print a formatted string to the kernel log
 *
 * @param fmt The format string
 * @param va The variable argument list
 *
 * @return The number of characters printed, if negative, an error happened
 */
int vprintk(const char* fmt, va_list va);


/**
 * @brief Print a formatted string to the kernel log
 *
 * @param fmt The format string
 * @param ... The variable arguments for the format string
 *
 * @return The number of characters printed, if negative, an error happened
 */
__attribute__((format(printf, 1, 2)))
int printk(const char* fmt, ...);
