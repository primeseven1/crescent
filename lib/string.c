#include <crescent/string.h>

size_t strlen(const char* str)
{
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

void* memset(void* dest, int val, size_t count)
{
    u8* d = dest;
    while (count--)
        *d++ = (u8)val;
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count)
{
    u8* d = dest;
    const u8* s = src;

    while (count--)
        *d++ = *s++;

    return dest;
}

void* memmove(void* dest, const void* src, size_t count)
{
    u8* d = dest;
    const u8* s = src;

    if (d < s) {
        while (count--)
            *d++ = *s++;
    } else {
        d += count;
        s += count;

        while (count--)
            *--d = *--s;
    }

    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, size_t count)
{
    const u8* p1 = ptr1;
    const u8* p2 = ptr2;

    while (count--) {
        if (*p1 < *p2)
            return -1;
        else if (*p1 > *p2)
            return 1;

        p1++;
        p2++;
    }

    return 0;
}
