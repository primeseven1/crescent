#include <crescent/core/pmio.h>
#include "i8259.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA (PIC1_COMMAND + 1)
#define PIC2_COMMAND 0xA0
#define PIC2_DATA (PIC2_COMMAND + 1)

#define PIC_EOI 0x20

#define ICW1_ICW4 0x01
#define ICW1_SINGLE	0x02
#define ICW1_INTERVAL4 0x04
#define ICW1_LEVEL 0x08
#define ICW1_INIT 0x10

#define ICW4_8086 0x01
#define ICW4_AUTO 0x02
#define ICW4_BUF_SLAVE 0x08
#define ICW4_BUF_MASTER	0x0C
#define ICW4_SFNM 0x10

#define PIC_READ_IRR 0x0A
#define PIC_READ_ISR 0x0B

static inline u16 pic_read_irq_reg(u8 ocw3) {
    outb(PIC1_COMMAND, ocw3);
    outb(PIC2_COMMAND, ocw3);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

/* Hopefully these 2 functions do this correctly */

bool i8259_is_irq_spurious(u8 irq) {
    return ((pic_read_irq_reg(PIC_READ_ISR) & (1 << irq)) == 0);
}

void i8259_handle_spurious_irq(u8 irq) {
    /* Send an EOI to PIC1 since the PIC1 doesn't know the interrupt is fake */
    if (irq >= 8)
        outb(PIC1_COMMAND, PIC_EOI);
}

void i8259_send_eoi(u8 irq) {
	if (irq >= 8)
		outb(PIC2_COMMAND,PIC_EOI);
	
	outb(PIC1_COMMAND,PIC_EOI);
}

void i8259_mask_irq(u8 irq) {
    if (irq < 8) {
        u8 mask = inb(PIC1_DATA);
        mask |= 1 << irq;
        outb(PIC1_DATA, mask);
        return;
    }

    u8 mask = inb(PIC2_DATA);
    mask |= 1 << (irq - 8);
    outb(PIC2_DATA, mask);
}

void i8259_unmask_irq(u8 irq) {
    if (irq < 8) {
        u8 mask = inb(PIC1_DATA);
        mask &= ~(1 << irq);
        outb(PIC1_DATA, mask);
        return;
    }

    u8 mask = inb(PIC2_DATA);
    mask &= ~(1 << (irq - 8));
    outb(PIC2_DATA, mask);
}

void i8259_disable(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void i8259_init(void) {
    /* Start the initialization process in cascade mode, whatever the hell that means */
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
	pmio_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	pmio_wait();

    /* Set the PIC1 and PIC2 vector offsets */
	outb(PIC1_DATA, 0x20);
	pmio_wait();
	outb(PIC2_DATA, 0x28);
	pmio_wait();

    /* Tell PIC1 there is a PIC2 at IRQ2 */
	outb(PIC1_DATA, 4);
	pmio_wait();

    /* Tell PIC2 its cascade identity, whatever the hell that means */
	outb(PIC2_DATA, 2);
	pmio_wait();
	
    /* Tell the PIC's to use 8086 mode instead of 8080 mode */
	outb(PIC1_DATA, ICW4_8086);
	pmio_wait();
	outb(PIC2_DATA, ICW4_8086);
	pmio_wait();

    /* Mask every interrupt */
    i8259_disable();
}
