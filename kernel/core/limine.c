#include <crescent/core/limine.h>
#include <crescent/common.h>

__attribute__((section(".limine_requests_start_marker"), aligned(8), used))
static volatile u64 limine_requests_start_marker[4] = {
    0xf6b8f4b39de7d1ae, 0xfab91a6940fcb9cf,
    0x785c6ed015d3e316, 0x181e920a7852b9d9
};

/* Limine protocol base revision 2 */
__attribute__((section(".limine_requests"), used))
static volatile u64 limine_base_revision[3] = { 0xf9562b2d5c95a6c8, 0x6a7b384944536bdc, 2 };

__attribute__((section(".limine_requests")))
struct limine_stack_size_request g_limine_stack_size_request = {
    .request.id = LIMINE_STACK_SIZE_REQUEST,
    .request.revision = 0,
    .response = NULL,
    .stack_size = 0x4000
};

__attribute__((section(".limine_requests")))
struct limine_hhdm_request g_limine_hhdm_request = {
    .request.id = LIMINE_HHDM_REQUEST,
    .request.revision = 0,
    .response = NULL
};

__attribute__((section(".limine_requests")))
struct limine_framebuffer_request g_limine_framebuffer_request = {
    .request.id = LIMINE_FRAMEBUFFER_REQUEST,
    .request.revision = 0,
    .response = NULL
};

__attribute__((section(".limine_requests")))
struct limine_paging_mode_request g_limine_paging_mode_request = {
    .request.id = LIMINE_PAGING_MODE_REQUEST,
    .request.revision = 0,
    .response = NULL,
    .mode = LIMINE_PAGING_MODE_4LVL,
    .flags = 0
};

__attribute__((section(".limine_requests")))
struct limine_smp_request g_limine_smp_request = {
    .request.id = LIMINE_SMP_REQUEST,
    .request.revision = 0,
    .response = NULL,
    .flags = 0
};

__attribute__((section(".limine_requests")))
struct limine_mmap_request g_limine_mmap_request = {
    .request.id = LIMINE_MMAP_REQUEST,
    .request.revision = 0,
    .response = NULL
};

__attribute__((section(".limine_requests")))
struct limine_kernel_file_request g_limine_kernel_file_request = {
    .request.id = LIMINE_KERNEL_FILE_REQUEST,
    .request.revision = 0,
    .response = NULL
};

__attribute__((section(".limine_requests")))
struct limine_rsdp_request g_limine_rsdp_request = {
    .request.id = LIMINE_RSDP_REQUEST,
    .request.revision = 0,
    .response = NULL
};

__attribute__((section(".limine_requests")))
struct limine_smbios_request g_limine_smbios_request = {
    .request.id = LIMINE_SMBIOS_REQUEST,
    .request.revision = 0,
    .response = NULL
};

__attribute__((section(".limine_requests")))
struct limine_kernel_address_request g_limine_kernel_address_request = {
    .request.id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .request.revision = 0,
    .response = NULL,
};

__attribute__((section(".limine_requests_end_marker"), aligned(8), used))
static volatile u64 limine_requests_end_marker[2] = { 0xadc0e0531bb10d03, 0x9572709f31764c62 };
