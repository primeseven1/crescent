#pragma once

#include <crescent/types.h>
#include <crescent/asm/errno.h>

/**
 * @brief Convert a long long integer to a string
 *
 * @param dest Where to put the string
 * @param val The value to convert
 * @param base The base to convert the integer to (eg: 10 for decial, 16 for hexadecimal)
 * @param dsize The size of the destination buffer
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid base
 * @retval -EOVERFLOW Cannot fit the whole string inside the buffer
 */
int klltostr(char* dest, long long val, int base, size_t dsize);

/**
 * @brief Convert an unsigned long long integer to a string
 *
 * @param dest Where to put the string
 * @param val The value to convert
 * @param base The base to convert the integer to (eg: 10 for decial, 16 for hexadecimal)
 * @param dsize The size of the destination buffer
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid base
 * @retval -EOVERFLOW Cannot fit the whole string inside the buffer
 */
int kulltostr(char* dest, unsigned long long val, int base, size_t dsize);
