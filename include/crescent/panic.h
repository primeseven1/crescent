#pragma once

/*
 * Crash the system
 *
 * --- Parameters ---
 * fmt - Format string for panic message
 */
__attribute__((format(printf, 1, 2)))
_Noreturn void panic(const char* fmt, ...);
