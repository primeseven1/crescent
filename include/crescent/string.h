#pragma once

#include <crescent/types.h>

/*
 * Get the length of a null terminated string
 *
 * --- Parameters ---
 * - str: The string
 *
 * -- Return values ---
 * - The length
 */
size_t strlen(const char* str);

/*
 * Set a block of memory to a value
 *
 * --- Parameters ---
 * - dest: Destination
 * - val: The value
 * - count: The amount of bytes
 *
 * --- Return values ---
 * - dest
 */
void* memset(void* dest, int val, size_t count);

/*
 * Copy a block of memory
 *
 * --- Notes ---
 * - Does not handle overlapping memory regions
 *
 * --- Parameters ---
 * - dest: Destination
 * - val: The value
 * - count: The amount of bytes
 *
 * -- Return values ---
 * - dest
 */
void* memcpy(void* dest, const void* src, size_t count);

/*
 * Copy a block of memory
 *
 * --- Notes ---
 * - Unlike memcpy, this handles overlapping memory regions
 *
 * --- Parameters ---
 * - dest: Destination
 * - val: The value
 * - count: The amount of bytes
 *
 * -- Return values ---
 * - dest
 */
void* memmove(void* dest, const void* src, size_t count);

/*
 * Compare 2 memory blocks
 *
 * --- Parameters ---
 * - ptr1: First pointer
 * - ptr2: Second pointer
 * - count: The amount of bytes
 *
 * --- Return values ---
 * - 1: The thing pointed to at the byte compared by ptr1 is more than ptr2's byte
 * - -1: The thing pointed to at the byte compared by ptr1 is less than ptr2's byte
 * - 0: Memory blocks are equal
 */
int memcmp(const void* ptr1, const void* ptr2, size_t count);
