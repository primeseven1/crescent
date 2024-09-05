OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

PHDRS {
    text PT_LOAD; /* Execute + Read */
    rodata PT_LOAD; /* Read only */
    data PT_LOAD; /* Write + Read */
    dynamic PT_DYNAMIC; /* Write + Read */
}

SECTIONS {
    . = 0;

    _lds_kernel_start = .;

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .text : {
        _lds_kernel_text_start = .;
        *(.text .text.*)
        _lds_kernel_text_end = .;
    } :text

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : {
        _lds_kernel_rodata_start = .;
        *(.rodata .rodata.*)
        _lds_kernel_rodata_end = .;
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .exports : {
        _lds_kernel_exports_start = .;
        KEEP(*(.exports))
        _lds_kerne_exports_end = .;
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : {
        _lds_kernel_data_start = .;
        *(.data .data.*)
        KEEP(*(.limine_requests_start_marker))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end_marker))
        _lds_kernel_data_end = .;
    } :data

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .dynamic : {
        _lds_kernel_dynamic_start = .;
        *(.dynamic)
        _lds_kernel_dynamic_end = .;
    } :data :dynamic

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .bss : {
        _lds_kernel_bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _lds_kernel_bss_end = .;
    } :data

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }

    _lds_kernel_end = .;
}
