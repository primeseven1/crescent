#pragma once

#define MULTIBOOT2_HEADER_MAGIC 0xE85250D6
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

#define MULTIBOOT_HEADER_TAG_OPTIONAL 1
#define MULTIBOOT_HEADER_TAG_END 0
#define MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST 1
#define MULTIBOOT_HEADER_TAG_ADDRESS 2
#define MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS 3
#define MULTIBOOT_HEADER_TAG_CONSOLE_FLAGS 4
#define MULTIBOOT_HEADER_TAG_FRAMEBUFFER 5
#define MULTIBOOT_HEADER_TAG_MODULE_ALIGN 6
#define MULTIBOOT_HEADER_TAG_EFI_BOOT_SERVICES 7
#define MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI32 8
#define MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI64 9
#define MULTIBOOT_HEADER_TAG_RELOCATABLE 10

#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_CMDLINE 1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT_TAG_TYPE_MODULE 3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO 4
#define MULTIBOOT_TAG_TYPE_BOOTDEV 5
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_VBE 7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS 9
#define MULTIBOOT_TAG_TYPE_APM 10
#define MULTIBOOT_TAG_TYPE_EFI32 11
#define MULTIBOOT_TAG_TYPE_EFI64 12
#define MULTIBOOT_TAG_TYPE_SMBIOS 13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD 14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW 15
#define MULTIBOOT_TAG_TYPE_NETWORK 16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP 17
#define MULTIBOOT_TAG_TYPE_EFI_BOOT_SERVICES 18
#define MULTIBOOT_TAG_TYPE_EFI32_IMAGE_HANDLE 19
#define MULTIBOOT_TAG_TYPE_EFI64_IMAGE_HANDLE 20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR 21

#define MULTIBOOT_ARCHITECTURE_I386 0
#define MULTIBOOT_ARCHITECTURE_MIPS32 4

#ifndef __ASSEMBLY_FILE__

#include <crescent/types.h>

struct multiboot_tag {
    u32 type;
    u32 size;
} __attribute__((packed));

struct multiboot_info {
    u32 total_size;
    u32 reserved;
    struct multiboot_tag tags[0];
} __attribute__((packed));

struct multiboot_tag_string {
    struct multiboot_tag tag;
    char str[0];
} __attribute__((packed));

/* Framebuffer info */

struct multiboot_color {
    u8 red;
    u8 green;
    u8 blue;
} __attribute__((packed));

enum multiboot_framebuffer_types {
    MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED = 0,
    MULTIBOOT_FRAMEBUFFER_TYPE_RGB,
    MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT
};

struct multiboot_framebuffer_info {
    u64 addr;
    u32 pitch;
    u32 width;
    u32 height;
    u8 bpp;
    u8 type;
    u16 reserved;
} __attribute__((packed));

struct multiboot_framebuffer_palette {
    u16 num_colors;
    struct multiboot_color palette[0];
} __attribute__((packed));

struct multiboot_framebuffer_rgb {
    u8 red_field_pos;
    u8 red_mask_size;
    u8 green_field_pos;
    u8 green_mask_size;
    u8 blue_field_pos;
    u8 blue_mask_size;
} __attribute__((packed));

struct multiboot_tag_framebuffer {
    struct multiboot_tag tag;
    struct multiboot_framebuffer_info info;
    union {
        struct multiboot_framebuffer_palette palette;
        struct multiboot_framebuffer_rgb rgb;
    } __attribute__((packed));
} __attribute__((packed));

/* Memory map */

enum multiboot_mmap_types {
    MULTIBOOT_MEMORY_AVAILABLE = 1,
    MULTIBOOT_MEMORY_RESERVED,
    MULTIBOOT_MEMORY_ACPI_RECLAIMABLE,
    MULTIBOOT_MEMORY_NVS,
    MULTIBOOT_MEMORY_BADRAM
};

struct multiboot_mmap_entry {
    u64 addr;
    u64 len;
    u32 type;
    u32 zero;
} __attribute__((packed));

struct multiboot_tag_mmap {
    struct multiboot_tag tag;
    u32 entry_size;
    u32 entry_version;
    struct multiboot_mmap_entry entries[0];
} __attribute__((packed));

/* Boot device */

struct multiboot_tag_bootdev {
    struct multiboot_tag tag;
    u32 biosdev;
    u32 slice;
    u32 part;
} __attribute__((packed));

/* ACPI */

struct multiboot_tag_acpi {
    struct multiboot_tag tag;
    u8 rsdp[0];
} __attribute__((packed));

/* So that you don't have to search for the tags every time */
struct multiboot_tag_locations {
    const struct multiboot_tag_framebuffer* framebuffer;
    const struct multiboot_tag_mmap* mmap;
    const struct multiboot_tag_bootdev* boot_device;
    struct {
        const struct multiboot_tag_acpi* rsdp;
        bool new;
    } acpi;
    const struct multiboot_tag_string* bootloader_name;
};

#endif /* __ASSEMBLY_FILE__ */
