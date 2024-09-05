#pragma once

#include <crescent/types.h>

struct elf64_ehdr {
    unsigned char e_ident[16];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
};

enum elf64_ehdr_types {
    ET_NONE,
    ET_REL,
    ET_EXEC,
    ET_DYN,
    ET_CORE
};

enum elf64_ehdr_machines {
    EM_NONE,
    EM_X86_64 = 62
};

enum elf64_ehdr_versions {
    EV_NONE,
    EV_CURRENT
};

struct elf64_shdr {
    u32 sh_name;
    u32 sh_type;
    u64 sh_flags;
    u64 sh_addr;
    u64 sh_offset;
    u64 sh_size;
    u32 sh_link;
    u32 sh_info;
    u64 sh_addralign;
    u64 sh_entsize;
};

enum elf64_shdr_types {
    SHT_NULL,
    SHT_PROGBITS,
    SHT_SYMTAB,
    SHT_STRTAB,
    SHT_RELA,
    SHT_HASH,
    SHT_DYNAMIC,
    SHT_NOTE,
    SHT_NOBITS,
    SHT_REL,
    SHT_SHLIB,
    SHT_DYNSYM,
    SHT_INIT_ARRAY = 14,
    SHT_FINI_ARRAY,
    SHT_PREINIT_ARRAY,
    SHT_GROUP,
    SHT_SYMTAB_SHNDX
};

enum elf64_shdr_flags {
    SHF_WRITE = (1 << 0),
    SHF_ALLOC = (1 << 1),
    SHF_EXECINSTR = (1 << 2),
    SHF_MERGE = (1 << 4),
    SHF_STRINGS = (1 << 5),
    SHF_INFO_LINK = (1 << 6),
    SHF_LINK_ORDER = (1 << 7),
    SHF_OS_NONCONFORMING = (1 << 8),
    SHF_GROUP = (1 << 9),
    SHF_TLS = (1 << 10)
};

struct elf64_sym {
    u32 st_name;
    u8 st_info;
    u8 st_other;
    u16 st_shndx;
    u64 st_value;
    u64 st_size;
};

enum elf64_sym_bindings {
    STB_LOCAL,
    STB_GLOBAL,
    STB_WEAK
};

enum elf64_sym_types {
    STT_NOTYPE,
    STT_OBJECT,
    STT_FUNC,
    STT_SECTION,
    STT_FILE,
    STT_COMMON,
    STT_TLS
};

enum elf64_sym_visibility {
    STV_DEFAULT,
    STV_INTERNAL,
    STV_HIDDEN,
    STV_PROTECTED
};

struct elf64_rel {
    u64 r_offset;
    u64 r_info;
};

struct elf64_rela {
    u64 r_offset;
    u64 r_info;
    i64 r_addend;
};

enum elf64_rel_types {
    R_X86_64_NONE,
    R_X86_64_64,
    R_X86_64_GLOB_DAT = 0x06,
    R_X86_64_JUMP_SLOT,
    R_X86_64_RELATIVE,
};
