#include <crescent/asm/segment.h>
#include <crescent/string.h>

static struct gdt_struct bsp_gdt = { 0 };
static struct tss_struct bsp_tss = { 0 };

__attribute__((section(".bss"), aligned(16)))
static u8 bsp_tss_stack_rsp0[8192];
__attribute__((section(".bss"), aligned(16)))
static u8 bsp_tss_stack_ist1_mc[8192];
__attribute__((section(".bss"), aligned(16)))
static u8 bsp_tss_stack_ist2_df[8192];
__attribute__((section(".bss"), aligned(16)))
static u8 bsp_tss_stack_ist3_nmi[8192];

void do_idt_init(void);
void do_idt_load(void);

void do_bootstrap_processor_init(void)
{
    do_gdt_init(&bsp_gdt, &bsp_tss);

    bsp_tss.rsp0 = &bsp_tss_stack_rsp0[8192];
    bsp_tss.ist[0] = &bsp_tss_stack_ist1_mc[8192];
    bsp_tss.ist[1] = &bsp_tss_stack_ist2_df[8192];
    bsp_tss.ist[2] = &bsp_tss_stack_ist3_nmi[8192];
    bsp_tss.iopb = sizeof(bsp_tss);

    do_gdt_load(&bsp_gdt);

    do_idt_init();
    do_idt_load();
}
