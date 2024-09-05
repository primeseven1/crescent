#pragma once

/**
 * @brief Halt the system with a panic message
 *
 * @param fmt The format string
 * @param ... The variable arguments for the formatted string
 */
__attribute__((format(printf, 1, 2)))
_Noreturn void panic(const char* fmt, ...);
