#include <crescent/asm/cpuid.h>
#include <crescent/asm/ctl.h>

.code64
.text

.globl _start
.type _start, @function
_start:
    cli
    cld
    andq $-16, %rsp /* make sure the stack is aligned */
    call kernel_main
    ud2
.size _start, . - _start
