#include <crescent/common.h>
#include <crescent/core/printk.h>
#include <crescent/core/locking.h>
#include <crescent/core/debug/e9.h>
#include <crescent/lib/kxtostr.h>
#include <crescent/lib/string.h>
#include <crescent/lib/printf.h>

static char printk_buf[2048];
static const char* prefixes[] = {
    [PL_NONE_N] = "",
    [PL_EMERG_N] = "\033[31m[EMERG]\033[0m ", /* red */
    [PL_CRIT_N] = "\033[31m[CRIT]\033[0m ", /* red */
    [PL_ERR_N] = "\033[31m[ERR]\033[0m ", /* red */
    [PL_WARN_N] = "\033[33m[WARN]\033[0m ", /* yellow */
    [PL_INFO_N] = "\033[37m[INFO]\033[0m " /* white */
};
static spinlock_t lock = SPINLOCK_INITIALIZER;

static void 
default_printk_hook(__attribute__((unused)) const char* str, __attribute__((unused)) int len) {

}

static void (*printk_hook)(const char* str, int len) = default_printk_hook;

void printk_set_hook(void (*hook)(const char*, int)) {
    printk_hook = hook;
}

#ifdef CONFIG_DEBUG
static int printk_level = PL_INFO_N;
#else
static int printk_level = PL_ERR_N;
#endif

int set_printk_level(int level) {
    if (level > PL_DEFAULT_N)
        return -E2BIG;

    printk_level = level;
    return 0;
}

int vprintk(const char* fmt, va_list va) {
    int level;
    if (fmt[0] == PL_BEGIN_N) {
        if (__builtin_strlen(fmt) <= 2)
            return -EOVERFLOW;
        if (fmt[1] > PL_DEFAULT_N)
            level = PL_DEFAULT_N;
        else
            level = fmt[1];
        fmt += 2;
    } else {
        level = PL_DEFAULT_N;
    }

    const char* prefix = prefixes[level];
    int prefix_len = strlen(prefix);

    unsigned long flags;
    spinlock_lock_irq_save(&lock, &flags);
    int len = vsnprintf(printk_buf, sizeof(printk_buf), fmt, va);
    if (len > 0 && level <= printk_level) {
        printk_hook(prefix, prefix_len);
        printk_hook(printk_buf, len);
    }

#ifdef CONFIG_E9_ENABLE
    debug_e9_write_str(prefix, prefix_len);
    debug_e9_write_str(printk_buf, len);
#endif /* CONFIG_E9_ENABLE */

    spinlock_unlock_irq_restore(&lock, &flags);
    return len;
}

int printk(const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    int ret = vprintk(fmt, va);
    va_end(va);
    return ret;
}
