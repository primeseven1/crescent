ENTRY(_start)

KERNEL_VMA_OFFSET = 0xFFFFFF8000000000;

SECTIONS {
    . = 2M;

    _kernel_start_paddr = .;
    _kernel_start_vaddr = . + KERNEL_VMA_OFFSET;
    _kernel_readonly_start_paddr = .;
    _kernel_readonly_start_vaddr = . + KERNEL_VMA_OFFSET;

    .multiboot2.data : {
        KEEP(*(.multiboot2.data))
    }

    .early.text : {
        *(.early.text)
    }

    . += KERNEL_VMA_OFFSET;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA_OFFSET) {
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA_OFFSET) {
        *(.rodata)
    }


    _kernel_readonly_end_paddr = . - KERNEL_VMA_OFFSET;
    _kernel_readonly_end_vaddr = .;

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA_OFFSET) {
        *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA_OFFSET) {
        *(COMMON)
        *(.bss)
    }

    /* 
     * Align this so that grub doesn't try to put the multiboot info in the middle of a 
     * page where the kernel is mapped 
     */
    . = ALIGN(4K);
    _kernel_end_paddr = . - KERNEL_VMA_OFFSET;
    _kernel_end_vaddr = .;
}
