#include <crescent/common.h>
#include <crescent/asm/errno.h>
#include <crescent/core/ld_syms.h>
#include <crescent/core/limine.h>
#include <crescent/core/debug/trace.h>
#include <crescent/lib/elf.h>

static const struct elf64_sym* kernel_symtab = NULL;
static const char* kernel_syms_strtab = NULL;
static u32 kernel_sym_count = 0;

const char* trace_kernel_symbol_name(const void* addr) {
    if (!kernel_symtab || !kernel_syms_strtab)
        return NULL;
    if ((u8*)addr < &_lds_kernel_start || (u8*)addr > &_lds_kernel_end)
        return NULL;

    /* In the linker script, addresses start at 0, so we have to subtract KERNEL_MIN_VIRTADDR again */
    size_t kernel_offset = (uintptr_t)&_lds_kernel_start - KERNEL_MIN_VIRTADDR;
    addr = (u8*)addr - kernel_offset - KERNEL_MIN_VIRTADDR;

    for (u32 i = 0; i < kernel_sym_count; i++) {
        if ((uintptr_t)addr >= kernel_symtab[i].st_value &&
                (uintptr_t)addr < kernel_symtab[i].st_value + kernel_symtab[i].st_size)
            return kernel_syms_strtab + kernel_symtab[i].st_name;
    }

    return NULL;
}

ssize_t trace_kernel_symbol_offset(const void* addr) {
    if (!kernel_symtab)
        return -1;
    if ((u8*)addr < &_lds_kernel_start || (u8*)addr > &_lds_kernel_end)
        return -1;

    /* In the linker script, addresses start at 0, so we have to subtract KERNEL_MIN_VIRTADDR again */
    size_t kernel_offset = (uintptr_t)&_lds_kernel_start - KERNEL_MIN_VIRTADDR;
    addr = (u8*)addr - kernel_offset - KERNEL_MIN_VIRTADDR;

    for (u32 i = 0; i < kernel_sym_count; i++) {
        if ((uintptr_t)addr >= kernel_symtab[i].st_value &&
                (uintptr_t)addr < kernel_symtab[i].st_value + kernel_symtab[i].st_size)
            return (uintptr_t)addr - kernel_symtab[i].st_value;
    }

    return -1;
}

int tracing_init(void) {
    struct limine_kernel_file_response* kernel_file = g_limine_kernel_file_request.response;
    if (!kernel_file)
        return -ENOPROTOOPT;

    struct elf64_ehdr* ehdr = g_limine_kernel_file_request.response->kernel_file->addr;
    struct elf64_shdr* shdr_table = (struct elf64_shdr*)((u8*)ehdr + ehdr->e_shoff);

    /* Now find the symbol table second header and the string table associated with it */
    for (u16 i = 0; i < ehdr->e_shnum; i++) {
        if (shdr_table[i].sh_type != SHT_SYMTAB)
            continue;

        /* Check to see if the string table is available for the symbol table */
        u16 strtab_index = shdr_table[i].sh_link;
        if (shdr_table[strtab_index].sh_type != SHT_STRTAB)
            return -ENOENT;

        kernel_symtab = (struct elf64_sym*)((u8*)ehdr + shdr_table[i].sh_offset);
        kernel_syms_strtab = (char*)((u8*)ehdr + shdr_table[strtab_index].sh_offset);
        kernel_sym_count = shdr_table[i].sh_size / sizeof(struct elf64_sym);

        break;
    }

    if (!kernel_symtab)
        return -ENOENT;

    return 0;
}
