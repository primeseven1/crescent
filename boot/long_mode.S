.code32

.section .early.text, "ax"
.globl enable_long_mode

enable_long_mode:
    /* Enable PAE */
    movl %cr4, %eax
    orl $(1 << 5), %eax
    movl %eax, %cr4

    /* Set LME bit */
    movl $0xC0000080, %ecx
    rdmsr
    orl $(1 << 8), %eax
    wrmsr

    /* Enable paging + WP */
    movl %cr0, %eax
    orl $0x80010000, %eax
    movl %eax, %cr0

    ret
