.code32

#include <crescent/asm/multiboot2.h>
#include <crescent/asm/paging.h>

#define SERIAL_COM1 0x3F8

.section .rodata
bad_multiboot_magic: .string "Error: Bad Multiboot 2 magic number in eax\n\rSystem halted"
bad_multiboot_align: .string "Error: Bad Multiboot 2 information alignment\n\rSystem halted"

.section .early.text, "ax"
.globl do_early_checks

do_early_checks:
    /* In case of any errors, the direction flag needs to be cleared */
    cld

    /* multiboot2 sanity check */
    cmpl $MULTIBOOT2_BOOTLOADER_MAGIC, %eax
    jne bad_multiboot_magic_err
    /* check for alignment for multiboot info */
    testl $7, %ebx
    jnz bad_multiboot_align_err

    ret

bad_multiboot_magic_err:
    movw $SERIAL_COM1, %dx
    leal V2P(bad_multiboot_magic), %esi
    jmp print_err_loop

bad_multiboot_align_err:
    movw $SERIAL_COM1, %dx
    leal V2P(bad_multiboot_align), %esi

print_err_loop:
    /* Load a character into al from esi */
    lodsb

    /* Check for null character */
    cmpb $0, %al
    je early_check_hlt

    /* Output to the serial port */
    outb %al, %dx
    jmp print_err_loop

early_check_hlt:
    hlt
    jmp early_check_hlt

