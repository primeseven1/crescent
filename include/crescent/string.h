#pragma once

#include <crescent/types.h>

void* memset(void* dest, int val, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
void* memmove(void* dest, const void* src, size_t count);
int memcmp(const void* ptr1, const void* ptr2, size_t count);
