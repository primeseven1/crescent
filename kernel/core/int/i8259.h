#pragma once

#include <crescent/types.h>

/**
 * @brief Send an end of interrupt signal to the i8259 PIC
 * @param irq The IRQ that happened
 */
void i8259_send_eoi(u8 irq);

/**
 * @brief Mask an IRQ
 * @param irq The IRQ to mask
 */
void i8259_mask_irq(u8 irq);

/**
 * @brief Unmask an IRQ
 * @param irq The IRQ to unmask
 */
void i8259_unmask_irq(u8 irq);

/**
 * @brief Check if an interrupt that happened is real or not
 * @param irq The IRQ you want to check
 * @retval true The IRQ is a fake IRQ
 * @retval false The IRQ is a real IRQ
 */
bool i8259_is_irq_spurious(u8 irq);

/**
 * @brief Handle a spurious IRQ
 * @param irq The IRQ that happened
 */
void i8259_handle_spurious_irq(u8 irq);

/**
 * @brief Disable the 8259 PIC entirely
 *
 * This function simply masks all interrupts on the PIC. Note that spurious IRQ's 
 * can still happen even if all IRQ's are masked.
 */
void i8259_disable(void);

/**
 * @brief Initialize the intel 8259 PIC
 *
 * This will remap IRQ's to IDT entry 32-47.
 */
void i8259_init(void);
