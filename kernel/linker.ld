ENTRY(_start)

KERNEL_VMA_OFFSET = 0xC0000000;

SECTIONS {
    . = 2M;

    _kernel_start = .;
    _kernel_readonly_start = .;

    .multiboot2.rodata : {
        KEEP(*(.multiboot2.rodata))
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

    _kernel_readonly_end = .;

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA_OFFSET) {
        *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA_OFFSET) {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4096);
    _kernel_end = .;
}
