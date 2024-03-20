#include <crescent/asm/segment.h>
#include <crescent/panic.h>
#include <crescent/types.h>
#include "isr.h"

enum idt_flags {
    IDT_ATTRIBUTE_INTERRUPT = (0x0E << 0),
    IDT_ATTRIBUTE_TRAP = (0x0F << 0),
    IDT_ATTRIBUTE_DPL0 = 0,
    IDT_ATTRIBUTE_DPL1 = (1 << 5),
    IDT_ATTRIBUTE_DPL2 = (2 << 5),
    IDT_ATTRIBUTE_DPL3 = (3 << 5),
    IDT_ATTRIBUTE_PRESENT = (1 << 7)
};

struct idt_entry {
    u16 handler_low; /* Low 16 bits of the handler */
    u16 cs; /* Code segment */
    u8 ist; /* Interrupt stack table */
    u8 attributes; /* gate type, DPL, and present fields */
    u16 handler_mid; /* Middle 16 bits of the handler */
    u32 handler_high; /* High 32 bits of the handler */
    u32 reserved; /* Set to 0 */
} __attribute__((packed));

__attribute__((aligned(16)))
static struct idt_entry idt_entries[256] = { 0 };

static void (*exception_handlers[32])(void) = {
    _isr_0, _isr_1, _isr_2, _isr_3, _isr_4,_isr_5, _isr_6, _isr_7, _isr_8, _isr_9,
    _isr_10, _isr_11, _isr_12, _isr_13, _isr_14, _isr_15, _isr_16, _isr_17, _isr_18, _isr_19, 
    _isr_20, _isr_21, _isr_22, _isr_23, _isr_24, _isr_25, _isr_26, _isr_27, _isr_28, _isr_29,
    _isr_30, _isr_31
};

static void write_idt_entry(unsigned int index, void (*handler)(void), u16 cs, u8 ist, u8 attributes)
{
    /* Critical programming error, so just panic now to avoid issues later */
    if (unlikely(index >= ARRAY_SIZE(idt_entries)))
        panic("Invalid IDT entry index! index: %u", index);

    idt_entries[index].handler_low = (uintptr_t)handler & 0xFFFF;
    idt_entries[index].cs = cs;
    idt_entries[index].ist = ist;
    idt_entries[index].attributes = attributes;
    idt_entries[index].handler_mid = (uintptr_t)handler >> 16 & 0xFFFF;
    idt_entries[index].handler_high = (uintptr_t)handler >> 32;
    idt_entries[index].reserved = 0;
}

void do_idt_init(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(exception_handlers); i++) {
        switch (i) {
        case IDT_EXCEPTION_MACHINE_CHECK:
            write_idt_entry(i, exception_handlers[i], GDT_SEGMENT_KERNEL_CODE, 1,
                IDT_ATTRIBUTE_PRESENT | IDT_ATTRIBUTE_DPL0 | IDT_ATTRIBUTE_INTERRUPT);
            break;
        case IDT_EXCEPTION_DOUBLE_FAULT:
            write_idt_entry(i, exception_handlers[i], GDT_SEGMENT_KERNEL_CODE, 2,
                IDT_ATTRIBUTE_PRESENT | IDT_ATTRIBUTE_DPL0 | IDT_ATTRIBUTE_TRAP);
            break;
        case IDT_EXCEPTION_NMI:
            write_idt_entry(i, exception_handlers[i], GDT_SEGMENT_KERNEL_CODE, 3,
                IDT_ATTRIBUTE_PRESENT | IDT_ATTRIBUTE_DPL0 | IDT_ATTRIBUTE_INTERRUPT);
            break;
        default:
            write_idt_entry(i, exception_handlers[i], GDT_SEGMENT_KERNEL_CODE, 0, 
                IDT_ATTRIBUTE_PRESENT | IDT_ATTRIBUTE_DPL0 | IDT_ATTRIBUTE_TRAP);
            break;
        }
    }
}

void do_idt_load(void)
{
    struct {
        u16 limit;
        struct idt_entry* idt_ptr;
    } __attribute__((packed)) idtr = {
        .limit = sizeof(idt_entries) - 1,
        .idt_ptr = idt_entries
    };

    asm volatile("lidt %0" : : "m"(idtr));
}
