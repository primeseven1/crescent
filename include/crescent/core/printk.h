#pragma once

#include <crescent/types.h>

#define PL_BEGIN_N 1

#define PL_INFO_N 5
#define PL_WARN_N 4
#define PL_ERR_N 3
#define PL_CRIT_N 2
#define PL_EMERG_N 1
#define PL_NONE_N 0
#define PL_DEFAULT_N PL_INFO_N

#define PL_BEGIN_S "\001"

#define PL_INFO_S "\005"
#define PL_WARN_S "\004"
#define PL_ERR_S "\003"
#define PL_CRIT_S "\002"
#define PL_EMERG_S "\001"
#define PL_NONE_S "\000"

#define PL_INFO PL_BEGIN_S PL_INFO_S
#define PL_WARN PL_BEGIN_S PL_WARN_S
#define PL_ERR PL_BEGIN_S PL_ERR_S
#define PL_CRIT PL_BEGIN_S PL_CRIT_S
#define PL_EMERG PL_BEGIN_S PL_EMERG_S
#define PL_NONE PL_BEGIN_S PL_NONE_S

/**
 * @brief Set a hook for the printk function
 *
 * This function will set the hook for printk, which printk will call after
 * it's done handling the formatted string
 *
 * @param hook The function printk should use
 */
void printk_set_hook(void (*hook)(const char*, int));

/**
 * @brief Set the printk level
 *
 * The printk level will determine if the message gets logged to the screen or not.
 * If CONFIG_E9_ENABLE is enabled, messages will be printed to that no matter the log level.
 *
 * @param level What to set the log level to
 *
 * @retval -E2BIG The level is too big
 * @retval 0 Success
 */
int set_printk_level(int level);

/**
 * @brief Print a formatted string to the kernel log
 *
 * You can set the level by adding a PL_<LEVEL> at the beginning of the string.
 * This will determine if the message gets printed to the string or not.
 *
 * @param fmt The format string
 * @param va The variable argument list
 *
 * @return The number of characters printed, if negative, an error happened
 */
int vprintk(const char* fmt, va_list va);


/**
 * @brief Print a formatted string to the kernel log
 *
 * @param fmt The format string
 * @param ... The variable arguments for the format string
 *
 * @return The number of characters printed, if negative, an error happened
 */
__attribute__((format(printf, 1, 2)))
int printk(const char* fmt, ...);
