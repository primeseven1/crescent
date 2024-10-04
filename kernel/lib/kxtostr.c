#include <crescent/lib/kxtostr.h>
#include <crescent/lib/string.h>

static void strrev(char* str) {
    char* end = str + strlen(str) - 1;
    while (str < end) {
        char tmp = *str;
        *str++ = *end;
        *end-- = tmp;
    }
}

int klltostr(char* dest, long long val, int base, size_t dsize) {
    if (dsize == 0) {
        return -EOVERFLOW;
    } else if (dsize == 1) {
        *dest = '\0';
        return -EOVERFLOW;
    }

    if (base < 2 || base > 16) {
        *dest = '\0';
        return -EINVAL;
    }

    bool negative = val < 0;
    if (negative)
        val = -val;

    const char* characters = "0123456789abcdef";

    /* Do the actual conversion here */
    char* d = dest;
    do {
        *d++ = characters[val % base];
        val /= base;
    } while (--dsize > 1 && val);

    /* See if the conversion was actual successful or not */
    if (val && dsize < 2) {
        *d = '\0';
        return -EOVERFLOW;
    }

    if (negative) {
        if (dsize < 3) {
            *d = '\0';
            return -EOVERFLOW;
        }
        
        *d++ = '-';
    }

    *d = '\0';
    strrev(dest);
    return 0;
}

int kulltostr(char* dest, unsigned long long val, int base, size_t dsize) {
    if (dsize == 0) {
        return -EOVERFLOW;
    } else if (dsize == 1) {
        *dest = '\0';
        return -EOVERFLOW;
    }

    if (base < 2 || base > 16) {
        *dest = '\0';
        return -EINVAL;
    }

    const char* characters = "0123456789abcdef"; 

    /* Do the actual conversion */
    char* d = dest;
    do {
        *d++ = characters[val % base];
        val /= base;
    } while (--dsize > 1 && val);

    *d = '\0';

    /* Make sure the conversion was successful, unsigned is much simpler than signed :D */
    if (val && dsize < 2)
        return -EOVERFLOW;
 
    strrev(dest);
    return 0;
}
