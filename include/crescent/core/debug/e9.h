#pragma once

#ifdef CONFIG_E9_ENABLE

/**
 * @brief Write a character to port e9, a common port for virtual machines for debugging
 * @param c The character to write
 */
void debug_e9_write_c(char c);

/**
 * @brief Write a character to port e9, a common port for virtual machines for debugging
 *
 * @param str The string to write
 * @param len The length of the string
 */
void debug_e9_write_str(const char* str, int len);

#endif /* CONFIG_E9_ENABLE */
