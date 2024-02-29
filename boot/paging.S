.code32

#include <crescent/asm/paging.h>

.section .rodata
err_not_fully_mapped: .string "Error: The kernel is not fully mapped, create another page table\n\rSystem halted."

.section .bss
boot_pml4:
    .skip 4096
boot_pdpt:
    .skip 4096
boot_pd:
    .skip 4096
boot_pt1:
    .skip 4096

.section .early.text, "ax"
.globl map_higher_half

/*
 * Present + Writable: 0x03
 * Present: 0x01
 * PTE size: 8
 * Num PTE entries: 512
 */
map_higher_half:
    leal V2P(boot_pt1), %edi 
	movl $_kernel_start, %esi
	movl $512, %ecx

1:
    cmpl $V2P(_kernel_end), %esi
    jge 3f

    cmpl $_kernel_readonly_start, %esi
    jge .possibly_readonly 

.not_readonly:
    movl %esi, %edx
    orl $0x03, %edx
    movl %edx, (%edi)

    jmp 2f

.possibly_readonly:
    cmpl $V2P(_kernel_readonly_end), %esi
    jge .not_readonly

    movl %esi, %edx
    orl $0x01, %edx
    movl %edx, (%edi)

2:
    addl $PAGE_SIZE, %esi
    addl $8, %edi
    loop 1b

3:
    call verify_kernel_mapping

    /*
     * 0x200000 >> 21 & 0x01FF: 1
     * (0xFFFFFF8000000000 + 2MiB) >> 21 & 0x01FF: 1
     */
    leal V2P(boot_pt1), %eax
    orl $0x03, %eax
	movl %eax, V2P(boot_pd) + 1 * 8

    /*
     * 0x200000 >> 30 & 0x01FF: 0
     * (0xFFFFFF8000000000 + 2MiB) >> 30 & 0x01FF: 0
     */
    leal V2P(boot_pd), %eax
    orl $0x03, %eax
    movl %eax, V2P(boot_pdpt)

    /*
     * 0x200000 >> 39 & 0x01FF: 0
     * (0xFFFFFF8000000000 + 2MiB) >> 39 & 0x01FF: 511
     */
    leal V2P(boot_pdpt), %eax
    orl $0x03, %eax
    movl %eax, V2P(boot_pml4)
    movl %eax, V2P(boot_pml4) + 511 * 8

    leal V2P(boot_pml4), %eax
	movl %eax, %cr3

    ret 


verify_kernel_mapping:
    cmpl $0, %ecx
    je .possibly_not_fully_mapped

.fully_mapped:
    ret

.possibly_not_fully_mapped:
    cmpl $V2P(_kernel_end), %esi
    jge .fully_mapped

    leal V2P(err_not_fully_mapped), %esi
    jmp critical_boot_err

