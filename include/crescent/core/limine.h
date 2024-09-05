#pragma once

#include <crescent/types.h>

#define LIMINE_COMMON_MAGIC1 0xc7b1dd30df4c8b88
#define LIMINE_COMMON_MAGIC2 0x0a82e883a194f07b

#define LIMINE_STACK_SIZE_REQUEST { \
    LIMINE_COMMON_MAGIC1, LIMINE_COMMON_MAGIC2, \
    0x224ef0460a8e8926, 0xe1cb0fc25f46ea3d \
}
#define LIMINE_HHDM_REQUEST { \
    LIMINE_COMMON_MAGIC1, LIMINE_COMMON_MAGIC2, \
    0x48dcf1cb8ad2b852, 0x63984e959a98244b \
}
#define LIMINE_FRAMEBUFFER_REQUEST { \
    LIMINE_COMMON_MAGIC1, LIMINE_COMMON_MAGIC2, \
    0x9d5827dcd881dd75, 0xa3148604f6fab11b \
}
#define LIMINE_PAGING_MODE_REQUEST { \
    LIMINE_COMMON_MAGIC1, LIMINE_COMMON_MAGIC2, \
    0x95c1a0edab0944cb, 0xa4e5cb3842f7488a \
}
#define LIMINE_SMP_REQUEST { \
    LIMINE_COMMON_MAGIC1, LIMINE_COMMON_MAGIC2, \
    0x95a67b819a1b857e, 0xa0b61b723b6a73e0 \
}
#define LIMINE_MMAP_REQUEST { \
    LIMINE_COMMON_MAGIC1, LIMINE_COMMON_MAGIC2, \
    0x67cf3d9d378a806f, 0xe304acdfc50c3c62 \
}
#define LIMINE_KERNEL_FILE_REQUEST { \
    LIMINE_COMMON_MAGIC1, LIMINE_COMMON_MAGIC2, \
    0xad97e90e83f1ed67, 0x31eb5d1c5ff23b69 \
}
#define LIMINE_MODULE_REQUEST { \
    LIMINE_COMMON_MAGIC1, LIMINE_COMMON_MAGIC2, \
    0x3e7e279702be32af, 0xca1c4f3bd1280cee \
}
#define LIMINE_RSDP_REQUEST { \
    LIMINE_COMMON_MAGIC1, LIMINE_COMMON_MAGIC2, \
    0xc5e77b6b397e7b43, 0x27637845accdcf3c \
}
#define LIMINE_SMBIOS_REQUEST { \
    LIMINE_COMMON_MAGIC1, LIMINE_COMMON_MAGIC2, \
    0x9e9046f11e095391, 0xaa4a520fefbde5ee \
}
#define LIMINE_KERNEL_ADDRESS_REQUEST { \
    LIMINE_COMMON_MAGIC1, LIMINE_COMMON_MAGIC2, \
    0x71ba76863cc55f63, 0xb2644a48c516a487 \
}

struct limine_request {
    u64 id[4];
    u64 revision;
};

struct limine_stack_size_response {
    u64 revision;
};

struct limine_stack_size_request {
    struct limine_request request;
    struct limine_stack_size_response* response;
    u64 stack_size;
};

struct limine_hhdm_response {
    u64 revision;
    u64 offset;
};

struct limine_hhdm_request {
    struct limine_request request;
    struct limine_hhdm_response* response;
};

enum limine_video_memory_models {
    LIMINE_FRAMEBUFFER_RGB = 1
};

struct limine_video_mode {
    u64 pitch;
    u64 width;
    u64 height;
    u16 bpp;
    u8 memory_model;
    u8 red_mask_size;
    u8 red_mask_shift;
    u8 green_mask_size;
    u8 green_mask_shift;
    u8 blue_mask_size;
    u8 blue_mask_shift;
};

struct limine_framebuffer {
    void* addr;
    u64 width;
    u64 height;
    u64 pitch;
    u16 bpp;
    u8 memory_model;
    u8 red_mask_size;
    u8 red_mask_shift;
    u8 green_mask_size;
    u8 green_mask_shift;
    u8 blue_mask_size;
    u8 blue_mask_shift;
    u8 unused[7];
    u64 edid_size;
    void* edid;
    u64 mode_count;
    struct limine_video_mode** modes;
};

struct limine_framebuffer_response {
    u64 revision;
    u64 framebuffer_count;
    struct limine_framebuffer** framebuffers;
};

struct limine_framebuffer_request {
    struct limine_request request;
    struct limine_framebuffer_response* response;
};

enum limine_paging_modes {
    LIMINE_PAGING_MODE_4LVL,
    LIMINE_PAGING_MODE_5LVL,
};

struct limine_paging_mode_response {
    u64 revision;
    u64 mode;
    u64 flags;
};

struct limine_paging_mode_request {
    struct limine_request request;
    struct limine_paging_mode_response* response;
    u64 mode;
    u64 flags;
};

enum limine_smp_flags {
    LIMINE_SMP_FLAG_X2APIC = (1 << 0)
};

struct limine_smp_info {
    u32 processor_id;
    u32 lapic_id;
    u64 reserved;
    void (*goto_address)(struct limine_smp_info*);
    u64 extra_argument;
};

struct limine_smp_response {
    u64 revision;
    u32 flags;
    u32 bsp_lapic_id;
    u64 cpu_count;
    struct limine_smp_info** cpus;
};

struct limine_smp_request {
    struct limine_request request;
    struct limine_smp_response* response;
    u64 flags;
};

enum limine_mmap_types {
    LIMINE_MMAP_USABLE,
    LIMINE_MMAP_RESERVED,
    LIMINE_MMAP_ACPI_RECLAIMABLE,
    LIMINE_MMAP_NVS,
    LIMINE_MMAP_BAD_MEMORY,
    LIMINE_MMAP_BOOTLOADER_RECLAIMABLE,
    LIMINE_MMAP_KERNEL_AND_MODULES,
    LIMINE_MMAP_FRAMEBUFFER
};

struct limine_mmap_entry {
    void* base;
    u64 len;
    u64 type;
};

struct limine_mmap_response {
    u64 revision;
    u64 entry_count;
    struct limine_mmap_entry** entries;
};

struct limine_mmap_request {
    struct limine_request request;
    struct limine_mmap_response* response;
};

struct limine_uuid {
    u32 a;
    u16 b;
    u16 c;
    u8 d[8];
};

enum limine_media_types {
    LIMINE_MEDIA_TYPE_GENERIC,
    LIMINE_MEDIA_TYPE_OPTICAL,
    LIMINE_MEDIA_TYPE_TFTP
};

struct limine_file {
    u64 revision;
    void* addr;
    u64 size;
    char* path;
    char* cmdline;
    u32 media_type;
    u32 unused;
    u32 tftp_ip;
    u32 tftp_port;
    u32 partition_index;
    u32 mbr_disk_id;
    struct limine_uuid gpt_disk_uuid;
    struct limine_uuid gpt_part_uuid;
    struct limine_uuid part_uuid;
};

struct limine_kernel_file_response {
    u64 revision;
    struct limine_file* kernel_file;
};

struct limine_kernel_file_request {
    struct limine_request request;
    struct limine_kernel_file_response* response;
};

enum limine_module_flags {
    LIMINE_INTERNAL_MODULE_REQUIRED = (1 << 0),
    LIMINE_INTERNAL_MODULE_COMPRESSED = (1 << 1)
};

struct limine_module_response {
    u64 revision;
    u64 module_count;
    struct limine_file** modules;
};

struct limine_internal_module {
    const char* path;
    const char* cmdline;
    u64 flags;
};

struct limine_module_request {
    struct limine_request request;
    struct limine_module_response* response;
    u64 module_count;
    struct limine_internal_module** modules;
};

struct limine_rsdp_response {
    u64 revision;
    void* addr;
};

struct limine_rsdp_request {
    struct limine_request request;
    struct limine_rsdp_response* response;
};

struct limine_smbios_response {
    u64 revision;
    void* entry_32;
    void* entry_64;
};

struct limine_smbios_request {
    struct limine_request request;
    struct limine_smbios_response* response;
};

struct limine_kernel_address_response {
    u64 revision;
    void* phys_base;
    void* virt_base;
};

struct limine_kernel_address_request {
    struct limine_request request;
    struct limine_kernel_address_response* response;
};

extern struct limine_stack_size_request g_limine_stack_size_request;
extern struct limine_hhdm_request g_limine_hhdm_request;
extern struct limine_framebuffer_request g_limine_framebuffer_request;
extern struct limine_paging_mode_request g_limine_paging_mode_request;
extern struct limine_smp_request g_limine_smp_request;
extern struct limine_mmap_request g_limine_mmap_request;
extern struct limine_kernel_file_request g_limine_kernel_file_request;
extern struct limine_rsdp_request g_limine_rsdp_request;
extern struct limine_smbios_request g_limine_smbios_request;
extern struct limine_kernel_address_request g_limine_kernel_address_request;
