#pragma once

#include <crescent/types.h>
#include <crescent/asm/errno.h>

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
 * @brief Formats and stores a string into a buffer
 *
 * @param buf Where the string will be stored
 * @param bufsize The size of the buffer
 * @param fmt The format string
 * @param ... Variable arguments for the format string
 *
 * @return The number of characters written, negative if an error happens
 */
__attribute__((format(printf, 3, 4)))
int snprintf(char* buf, size_t bufsize, const char* fmt, ...);
