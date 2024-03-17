#pragma once

__attribute__((format(printf, 1, 2)))
_Noreturn void panic(const char* fmt, ...);
