#ifdef CONFIG_E9_ENABLE

#include <crescent/core/debug/e9.h>
#include <crescent/core/pmio.h>

void debug_e9_write_c(char c) {
    outb(0xe9, c);
}

void debug_e9_write_str(const char* str, int len) {
    while (len--)
        debug_e9_write_c(*str++);
}

#endif /* CONFIG_E9_ENABLE */
