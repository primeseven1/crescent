OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

PHDRS {
	text PT_LOAD; /* Execute + Read */
	rodata PT_LOAD; /* Read only */
	data PT_LOAD; /* Write + Read */
	dynamic PT_DYNAMIC; /* Write + Read */
}

SECTIONS {
	. = 0xffffffff80000000;

	INCLUDE "kernel/sections.ld"
}
