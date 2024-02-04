.code32

#include <crescent/asm/multiboot2.h>

.section .early.text, "ax"
.globl do_early_checks

do_early_checks:
    /* multiboot2 sanity check */
    cmpl $MULTIBOOT2_BOOTLOADER_MAGIC, %eax
    jne early_check_error
    /* check for alignment for multiboot info */
    testl $7, %ebx
    jnz early_check_error

    ret

early_check_error:
    hlt
    jmp early_check_error

