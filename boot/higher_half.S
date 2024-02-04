.code32

#include <crescent/asm/paging.h>

.section .bss
.align PAGE_DIRECTORY_ALIGN
boot_page_directory:
    .skip PAGE_DIRECTORY_SIZE
.align PAGE_TABLE_ALIGN
boot_page_table1:
    .skip PAGE_TABLE_SIZE

.section .early.text, "ax"
.globl map_higher_half

/* 
 * This function will work fine until the kernel is more than 2MiB, 
 * which i don't see happening for a little while 
 */
map_higher_half:
    /* 
     * the linker is gonna think we want the virtual address.
     * we don't, the CPU is still using physical addresses at this point 
     */
    leal V2P(boot_page_table1), %edi 

    /* The physical address */
	movl $0, %esi
    /* Mapping the whole page table */
	movl $1024, %ecx

1:
    /* 
     * _kernel_start and _kernel_readonly_start are already physical addresses, 
     * so no need to convert them 
     */
	cmpl $_kernel_start, %esi
	jl 2f
	cmpl $V2P(_kernel_end), %esi
	jge 3f

    cmpl $_kernel_readonly_start, %esi
    jge possibly_readonly

not_readonly:
    movl %esi, %edx
    /* present, writable */
    orl $0x003, %edx
    movl %edx, (%edi)

    jmp 2f

possibly_readonly:
    cmpl $V2P(_kernel_readonly_end), %esi
    jge not_readonly

	movl %esi, %edx
    /* present */
	orl $0x001, %edx
	movl %edx, (%edi)

2:
    /* Add the size of a page to get the next physical address we want to map */
	addl $PAGE_SIZE, %esi
    /* size of an entry in the page table is 4 bytes */
	addl $4, %edi
    /* Decrement ecx and loop back to the beginning of the loop, until ecx becomes 0, or in this context when kernel_end ends */
	loop 1b

3:
    /* Move the page table into the page directory, identity mapping and higher half mapping */
    leal V2P(boot_page_table1), %eax
    orl $0x03, %eax
    /* Entry 0 for virtual addresses 0 through 0x400000 */
	movl %eax, V2P(boot_page_directory)
    /* Entry 768 is for virtual addresses 0xC0000000 to 0xC0400000*/
	movl %eax, V2P(boot_page_directory) + 768 * 4

    /* cr3 needs the physical address of the page table before  */
	leal V2P(boot_page_directory), %eax
	movl %eax, %cr3

    /* Now enable paging and write-protect bits in cr0 */
	movl %cr0, %eax
	orl $0x80010000, %eax
	movl %eax, %cr0

    ret

