#include <crescent/kernel.h>
#include <crescent/kprint.h>

_Noreturn void panic(const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    DBG_E9_VKPRINT(fmt, va);

    va_end(va);

    while (1)
        asm volatile("hlt");
}
