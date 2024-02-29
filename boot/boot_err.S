.code32

#define SERIAL_COM1 0x3F8

.section .early.text, "ax"
.globl critical_boot_err

/* ESI: the physical address of the string */
critical_boot_err:
    cld
    movw $SERIAL_COM1, %dx
.print_err_loop:
    lodsb

    cmpb $0, %al
    je err_hlt

    outb %al, %dx
    jmp .print_err_loop

err_hlt:
    hlt
    jmp err_hlt
