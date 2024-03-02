#include <crescent/conversions.h>
#include <crescent/string.h>

static inline void strrev(char* str)
{
    char* end = str + strlen(str) - 1;
    while (str < end) {
        char tmp = *str;
        *str++ = *end;
        *end-- = tmp;
    }
}

int lltostr(long long val, char* dest, int base, size_t dsize)
{
    if (base > 36 || base < 2 || dsize < 2)
        return -E2BIG;

    bool negative = false;
    if (val < 0) {
        negative = true;
        val = -val;
    }

    const char* characters = "0123456789abcdefghijklmnopqrstuvwxyz";

    char* d = dest;
    do {
        *d++ = characters[val % base];
        val /= base;
    } while (--dsize > 1 && val);

    if (val && dsize < 2) {
        *d = '\0';
        return -E2BIG;
    }

    if (negative)
        *d++ = '-';

    *d = '\0';
    strrev(dest);

    return 0;
}

int ulltostr(unsigned long long val, char* dest, int base, size_t dsize)
{
    if (base > 36 || base < 2 || dsize < 2)
        return -E2BIG;

    const char* characters = "0123456789abcdefghijklmnopqrstuvwxyz"; 

    char* d = dest;
    do {
        *d++ = characters[val % base];
        val /= base;
    } while (--dsize > 1 && val);

    *d = '\0';

    if (val && dsize < 2)
        return -E2BIG;
 
    strrev(dest);

    return 0;
}

int dtostr(double val, char* dest, int afterpoint, size_t dsize)
{
    long long int_part = (long long)val;
    double decimal_part = val < 0 ? -val - (long long)-val : val - (long long)val;
    
    /* This function will check the arguments already, so just return the error it returns */
    int err = lltostr(int_part, dest, 10, dsize);
    if (err)
        return err;

    if (!afterpoint)
        return 0;

    size_t len = strlen(dest);
    dest += len;
    dsize -= len;

    /* Now make sure there is enough room after the first conversion is done */
    if (dsize < 2)
        return -E2BIG;

    int ret = 0;

    /* Now time to convert the decimal part */
    *dest++ = '.';
    dsize--;
    while (afterpoint--) {
        if (dsize < 2) {
            ret = -E2BIG;
            break;
        }

        decimal_part *= 10;
        int digit = (int)decimal_part;
        *dest++ = '0' + digit;
        decimal_part -= digit;
        dsize--;
    }

    *dest = '\0';
    return ret;
}
