#pragma once

#include <crescent/types.h>

void i8259_send_eoi(u8 irq);
void i8259_mask_irq(u8 irq);
void i8259_unmask_irq(u8 irq);
bool i8259_is_irq_spurious(u8 irq);
void i8259_handle_spurious_irq(u8 irq);
void i8259_disable(void);
void i8259_init(void);
