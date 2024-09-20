#include <crescent/lib/kstrtox.h>
#include <crescent/limits.h>

static unsigned int determine_base(const char* s) {
    if (*s == '0') {
        if (s[1] == 'x' || s[1] == 'X')
            return 16;
        else if (s[1] == 'b' || s[1] == 'B')
            return 2;
        return 8;
    }

    return 10;
}

int kstrtoll(const char* s, size_t maxlen, unsigned int base, long long* res) {
    *res = 0;
    while (*s == ' ')
        s++;

    int sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    /* 
     * If the base has to be determined automatically, then we know what the string starts with.
     * If not, then we have to check it manually, since the base was provided by the user.
     */
    if (base == 0) {
        base = determine_base(s);
        if (base == 2 || base == 16)
            s += 2;
        else if (base == 8)
            s++;
    } else {
        if ((base == 16 && (*s == '0' && (s[1] == 'x' || s[1] == 'X'))) ||
                (base == 2 && (*s == '0' && (s[1] == 'b' || s[1] == 'B'))))
            s += 2;
        else if (base == 8 && *s == '0')
            s++;
    }

    long long max_limit = LLONG_MAX / base;
    while (maxlen-- && *s) {
        long long digit = -1;
        switch (base) {
        case 16:
            if (*s >= 'a' && *s <= 'f') {
                digit = *s - 'a' + 10;
                break;
            } else if (*s >= 'A' && *s <= 'F') {
                digit = *s - 'A' + 10;
                break;
            }
        /* fall through */
        case 10:
            if (*s >= '0' && *s <= '9')
                digit = *s - '0';
            break;
        case 8:
            if (*s >= '0' && *s <= '7')
                digit = *s - '0';
            break;
        case 2:
            if (*s == '0' || *s == '1')
                digit = *s - '0';
            break;
        default:
            return -EINVAL;
        }

        if (digit == -1)
            return -EINVAL;
        if (*res > max_limit || (*res == max_limit && digit > LLONG_MAX % base))
            return -ERANGE;

        *res = *res * base + digit;
        s++;
    }

    *res *= sign;
    return 0;
}

int kstrtoull(const char* s, size_t maxlen, unsigned int base, unsigned long long* res) {
    *res = 0;
    while (*s == ' ')
        s++;
    if (*s == '+') {
        s++;
    }

    /* 
     * If the base has to be determined, then we already know how far to 
     * jump into the string before getting to the actual number 
     */
    if (base == 0) {
        base = determine_base(s);
        if (base == 2 || base == 16)
            s += 2;
        else if (base == 8)
            s++;
    } else {
        if ((base == 16 && (*s == '0' && (s[1] == 'x' || s[1] == 'X'))) ||
                (base == 2 && (*s == '0' && (s[1] == 'b' || s[1] == 'B'))))
            s += 2;
        else if (base == 8 && *s == '0')
            s++;
    }

    unsigned long long max_limit = ULLONG_MAX / base;

    while (maxlen-- && *s) {
        unsigned long long digit = ULLONG_MAX;
        switch (base) {
        case 16:
            if (*s >= 'a' && *s <= 'f') {
                digit = *s - 'a' + 10;
                break;
            } else if (*s >= 'A' && *s <= 'F') {
                digit = *s - 'A' + 10;
                break;
            }
        /* fall through */
        case 10:
            if (*s >= '0' && *s <= '9')
                digit = *s - '0';
            break;
        case 8:
            if (*s >= '0' && *s <= '7')
                digit = *s - '0';
            break;
        case 2:
            if (*s == '0' || *s == '1')
                digit = *s - '0';
            break;
        default:
            return -EINVAL;
        }

        if (digit == ULLONG_MAX)
            return -EINVAL;
        if (*res > max_limit || (*res == max_limit && digit > ULLONG_MAX % base))
            return -ERANGE;

        *res = *res * base + digit;
        s++;
    }

    return 0;
}
