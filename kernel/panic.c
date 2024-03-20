#include <crescent/kernel.h>
#include <crescent/kprint.h>

_Noreturn void panic(const char* fmt, ...)
{
    asm volatile("cli");

    va_list va;
    va_start(va, fmt); 

    DBG_E9_KPRINT_NOFMT("Kernel panic - ");
    DBG_E9_VKPRINT(fmt, va);
    DBG_E9_KPRINT_NOFMT("\n");

    va_end(va);

    while (1)
        asm volatile("hlt");
}
