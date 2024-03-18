#pragma once

#include <crescent/types.h>
#include <crescent/kernel.h>

/* Any error means the value in dest is garbage, do not use the value if an error is returned */

/* 
 * Convert a long long to string 
 *
 * --- Parameters ---
 * val - The value
 * dest - The destination
 * base - The number base
 * dsize - size of the destination
 *
 * --- Return values ---
 * - -E2BIG: val is too big to fit into the destination
 * - 0: Success
 */
int lltostr(long long val, char* dest, int base, size_t dsize);

/*
 * Convert an unsigned long long to a string
 *
 * --- Parameters ---
 * val - The value
 * dest - The destination
 * base - The number base
 * dsize - Destination size
 *
 * --- Return values ---
 * - -E2BIG: val is too big to fit into the destination
 * - 0: Success
 */
int ulltostr(unsigned long long val, char* dest, int base, size_t dsize);

/*
 * Convert a double into a string
 *
 * --- Parameters ---
 * val - The value
 * dest - The destination
 * afterpoint - The amount of presecion after the decimal point
 * disze - Destination size
 *
 * --- Return values ---
 * - -E2BIG: val is too big to fit into the destination
 * - 0: Success
 */
int dtostr(double val, char* dest, int afterpoint, size_t dsize);
