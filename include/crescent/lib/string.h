#pragma once

#include <crescent/types.h>

/**
 * @brief Get the length of a null terminated string
 * @param str The string
 * @return The length of the string
 */
size_t strlen(const char* str);

/**
 * @brief Copy a string from one location to another
 *
 * This function is unsafe because it does not pay attention to
 * buffer sizes, use strncpy or strlcpy instead.
 *
 * @param dest Where to put the string
 * @param src The string to copy
 *
 * @return The pointer to dest
 */
char* strcpy(char* dest, const char* src);

/**
 * @brief Copy a string from one location to another
 *
 * @param dest Where to put the string
 * @param src The string to copy
 * @param count The maximum number of characters to copy
 *
 * @return The pointer to dest
 */
char* strncpy(char* dest, const char* src, size_t count);

/**
 * @brief Copy a string from one location to another
 *
 * @param dest Where to put the string
 * @param src The string to copy
 * @param dsize The size of the destination buffer
 *
 * @return The length of the source string
 */
size_t strlcpy(char* dest, const char* src, size_t dsize);

/**
 * @brief Concatenate 2 strings
 *
 * Like strcpy, this function does not pay attention to buffer sizes, so use
 * strncat or strlcat instead.
 *
 * @param dest The string to add the string on to
 * @param src The string to add
 *
 * @return The pointer to dest
 */
char* strcat(char* dest, const char* src);

/**
 * @brief Concatenate 2 strings
 *
 * Since this function takes the maximum amount of characters to add rather than a buffer
 * size, you need to get the length of the destination string and then use that to determine
 * the amount of characters to add. So it's still possible to overrun the buffer. An alternative
 * to this function is strlcat, which will handle that for you.
 *
 * @param dest The string to add the string on to
 * @param src The string to add
 * @param count The maximum amount of characters to add
 *
 * @return The pointer to dest
 */
char* strncat(char* dest, const char* src, size_t count);

/**
 * @brief Concatenate 2 strings
 *
 * @param dest The string to add the string on to
 * @param src The string to add
 * @param dsize The size of the destination buffer
 *
 * @return The initial length of dest + length of src
 */
size_t strlcat(char* dest, const char* src, size_t dsize);

/**
 * @brief Compare 2 strings
 * @param s1,s2 The strings to compare
 * @return 0 if equal, negative if *s1 < *s2, positive if *s1 > *s2 (last compared character)
 */
int strcmp(const char* s1, const char* s2);

/**
 * @brief Compare 2 strings
 *
 * @param s1,s2 The strings to compare
 * @param count The maximum amount of characters to compare
 *
 * @return 0 if equal, negative if *s1 < *s2, positive if *s1 > *s2 (last compared character)
 */
int strncmp(const char* s1, const char* s2, size_t count);

/**
 * @brief Set a block of memory to a value
 *
 * @param dest The block of memory
 * @param val The value to set the block to, should be an 8 bit value
 * @param count The amount of bytes to set
 *
 * @return The pointer to dest
 */
void* memset(void* dest, int val, size_t count);

/**
 * @brief Copy a block of memory from one location to another
 *
 * @param dest Where to copy the memory to
 * @param src The block of memory to copy from
 * @param count The number of bytes to copy
 *
 * @return The pointer to dest
 */
void* memcpy(void* dest, const void* src, size_t count);

/**
 * @brief Copy a block of memory from one location to another
 *
 * Unlike memcpy, this function does handle overlapping memory regions.
 * So if you know the memory regions overlap or could overlap, use this function.
 *
 * @param dest Where to copy the memory to
 * @param src The block of memory to copy from
 * @param count The number of bytes to copy
 *
 * @return The pointer to dest
 */
void* memmove(void* dest, const void* src, size_t count);

/**
 * @brief Compare 2 blocks of memory
 * @param b1,b2 The blocks of memory to compare
 * @param count The amount of bytes to compare
 * @return 0 if equal, negative if *b1 < *b2, positive if *b1 > *b2 (last compared byte)
 */
int memcmp(const void* b1, const void* b2, size_t count);
