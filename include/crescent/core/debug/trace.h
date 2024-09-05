#pragma once

#include <crescent/types.h>

#define KERNEL_MIN_VIRTADDR 0xFFFFFFFF80000000

/**
 * @brief Retrieves the name of the kernel symbol associated with an address
 * @param addr The address
 * @return NULL if there is no symbol, otherwise you will get the symbol name
 */
const char* trace_kernel_symbol_name(const void* addr);

/**
 * @brief Try to get the offset of an address from the start of the kernel symbol
 * @param addr The address
 * @return -1 if the address does not correspond to any symbol, otherwise it returns the offset
 */
ssize_t trace_kernel_symbol_offset(const void* addr);

/**
 * @brief Initialize the tracing system
 * @retval -ENOPROTOOPT The bootloader did not provide the kernel ELF
 * @retval -ENOENT The symbol table could not be found
 */
int tracing_init(void);
