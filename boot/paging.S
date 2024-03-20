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

map_higher_half:
    leal V2P(boot_pt1), %edi 
	movl $_kernel_start_paddr, %esi
	movl $512, %ecx

1:
    cmpl $_kernel_end_paddr, %esi
    jge 3f

    cmpl $_kernel_readonly_start_paddr, %esi
    jge .possibly_readonly 

.not_readonly:
    movl %esi, %edx
    /* present/writable */
    orl $0x03, %edx
    movl %edx, (%edi)

    jmp 2f

.possibly_readonly:
    cmpl $_kernel_readonly_end_paddr, %esi
    jge .not_readonly

    movl %esi, %edx
    /* present */
    orl $0x01, %edx
    movl %edx, (%edi)

2:
    addl $PAGE_SIZE, %esi
    /* page table entry size: 8 */
    addl $8, %edi
    loop 1b

3:
    call verify_kernel_mapping

    /* identity map kernel */
    leal V2P(boot_pt1), %eax
    orl $0x03, %eax
	movl %eax, V2P(boot_pd) + 1 * 8

    leal V2P(boot_pd), %eax
    orl $0x03, %eax
    movl %eax, V2P(boot_pdpt)

    leal V2P(boot_pdpt), %eax
    orl $0x03, %eax
    movl %eax, V2P(boot_pml4)

    /* map to higher half */
    movl %eax, V2P(boot_pml4) + 511 * 8

    /* doing recursive mapping at entry 510 since the kernel is mapped at 511 */
    leal V2P(boot_pml4), %eax
    orl $0x03, %eax
    movl %eax, V2P(boot_pml4) + 510 * 8

    leal V2P(boot_pml4), %eax
	movl %eax, %cr3

    ret 


verify_kernel_mapping:
    cmpl $0, %ecx
    je .possibly_not_fully_mapped

.fully_mapped:
    ret

.possibly_not_fully_mapped:
    cmpl $_kernel_end_paddr, %esi
    jge .fully_mapped

    leal V2P(err_not_fully_mapped), %esi
    jmp critical_boot_err
