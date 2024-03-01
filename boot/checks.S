.code32

#include <crescent/asm/multiboot2.h>
#include <crescent/asm/paging.h>

.section .rodata
bad_multiboot_magic: .string "Critical Error: Not loaded by a Multiboot 2 bootloader\n"
bad_multiboot_align: .string "Critical Error: Bad Multiboot 2 information alignment\n"
bad_cpuid: .string "Critical Error: CPU doesn't support CPUID\n"
bad_long_mode: .string "Critical Error: CPU doesn't support long mode\n"
bad_sse_mmx: .string "Critical Error: SSE/MMX not supported by the CPU\n"

.section .early.text, "ax"
.globl do_early_checks

do_early_checks:
    cmpl $MULTIBOOT2_BOOTLOADER_MAGIC, %eax
    jne bad_multiboot_magic_err
    testl $7, %ebx
    jnz bad_multiboot_align_err
.cpuid_check:
    pushfl
    popl %eax
    movl %eax, %ecx
    xorl $(1 << 21), %eax
    pushl %eax
    popfl
    pushfl
    popl %eax
    pushl %ecx
    popfl
    cmpl %ecx, %eax
    je bad_cpuid_err
    /* ecx can be used for input with cpuid, so set this to 0 */
    xorl %ecx, %ecx
.long_mode_check:
    movl $0x80000000, %eax
    cpuid
    cmpl $0x80000001, %eax
    jb bad_long_mode_err
    movl $0x80000001, %eax
    cpuid
    testl $(1 << 29), %edx
    jz bad_long_mode_err
.sse_check:
    movl $0x01, %eax
    cpuid
    /* SSE1 */
    testl $(1 << 25), %edx
    jz bad_sse_mmx_err
    /* SSE2 */
    testl $(1 << 26), %edx
    jz bad_sse_mmx_err
    /* MMX */
    testl $(1 << 23), %edx
    jz bad_sse_mmx_err
.done:
    ret


bad_multiboot_magic_err:
    leal V2P(bad_multiboot_magic), %esi
    jmp critical_boot_err
bad_multiboot_align_err:
    leal V2P(bad_multiboot_align), %esi
    jmp critical_boot_err
bad_cpuid_err:
    leal V2P(bad_cpuid), %esi
    jmp critical_boot_err
bad_long_mode_err:
    leal V2P(bad_long_mode), %esi
    jmp critical_boot_err
bad_sse_mmx_err:
    leal V2P(bad_sse_mmx), %esi
    jmp critical_boot_err

