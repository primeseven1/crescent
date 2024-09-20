#pragma once

#include <crescent/types.h>
#include <crescent/asm/errno.h>

/**
 * @brief Convert a string to a long long integer
 *
 * This function supports converting binary, octal, decimal, and hexadecimal integers.
 * If this returns an error, assume that the result is garbage.
 *
 * @param[in] s The string to convert
 * @param[in] maxlen The maximum amount of characters to check
 * @param[in] base The base to convert to, input 0 for automatic
 * @param[out] res The pointer to where you want to store the result
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid base, or invalid character
 * @retval -ERANGE Conversion would result in an integer overflow
 */
int kstrtoll(const char* s, size_t maxlen, unsigned int base, long long* res);
int kstrtol(const char* s, size_t maxlen, unsigned int base, long* res);

/**
 * @brief Convert a string to an unsigned long long integer
 *
 * This function supports converting binary, octal, decimal, and hexadecimal integers.
 * If this returns an error, assume that the result is garbage.
 *
 * @param[in] s The string to convert
 * @param[in] maxlen The maximum amount of characters you want to check
 * @param[in] base The base to convert to, input 0 for automatic
 * @param[out] res The pointer to where you want to store the result
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid base, or invalid character
 * @retval -ERANGE Conversion would result in an integer overflow
 */
int kstrtoull(const char* s, size_t maxlen, unsigned int base, unsigned long long* res);
int kstrtoul(const char* s, size_t maxlen, unsigned int base, unsigned long* res);
