#include <crescent/kernel.h>
#include <crescent/kprint.h>

_Noreturn void panic(const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    DBG_E9_KPRINT_NOFMT("Kernel panic - ");
    DBG_E9_KPRINT(fmt, va);

    va_end(va);

    while (1)
        asm volatile("hlt");
}
