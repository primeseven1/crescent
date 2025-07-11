#include <crescent/types.h>
#include <crescent/common.h>
#include <crescent/core/cpu.h>
#include <crescent/core/io.h>
#include <crescent/core/apic.h>
#include <crescent/core/printk.h>
#include <crescent/core/panic.h>
#include <crescent/core/timekeeper.h>
#include "sched.h"

#define WAIT_LENGTH 10000u

static u32 sched_lapic_timer_get_ticks_per_10ms(void) {
	lapic_write(LAPIC_REG_TIMER_DIVIDE, 0x03); /* Set the divisor to 16 */
	lapic_write(LAPIC_REG_TIMER_INITIAL, U32_MAX); /* Set the initial count to the maximum */
	timekeeper_stall(WAIT_LENGTH);
	lapic_write(LAPIC_REG_LVT_TIMER, 1 << 16); /* Stop timer */

	/* Now just return the difference of the initial count and the current count */
	return U32_MAX - lapic_read(LAPIC_REG_TIMER_CURRENT); 
}

static void timer(const struct isr* isr, struct context* ctx) {
	(void)isr;
	(void)ctx;
	sched_switch_from_interrupt(ctx);
}

static struct irq timer_irq = {
	.irq = -1,
	.eoi = apic_eoi
};

static const struct isr* lapic_timer_isr = NULL;

void sched_lapic_timer_init(void) {
	if (!lapic_timer_isr) {
		lapic_timer_isr = interrupt_register(&timer_irq, timer);
		assert(lapic_timer_isr != NULL);
	}

	u32 ticks = sched_lapic_timer_get_ticks_per_10ms();
	lapic_write(LAPIC_REG_LVT_TIMER, lapic_timer_isr->vector | (1 << 17));
	lapic_write(LAPIC_REG_TIMER_DIVIDE, 0x03);
	lapic_write(LAPIC_REG_TIMER_INITIAL, ticks);

	printk(PRINTK_DBG "sched: LAPIC timer calibrated at %u ticks per %u us on CPU %u\n", 
			ticks, WAIT_LENGTH, current_cpu()->processor_id);
}
