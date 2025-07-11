#include <crescent/compiler.h>
#include <crescent/asm/wrap.h>
#include <crescent/init/status.h>
#include <crescent/core/limine.h>
#include <crescent/core/module.h>
#include <crescent/core/trace.h>
#include <crescent/core/printk.h>
#include <crescent/core/panic.h>
#include <crescent/core/cpu.h>
#include <crescent/core/cmdline.h>
#include <crescent/core/term.h>
#include <crescent/core/interrupt.h>
#include <crescent/core/apic.h>
#include <crescent/core/timekeeper.h>
#include <crescent/mm/buddy.h>
#include <crescent/mm/heap.h>
#include <crescent/asm/segment.h>
#include <crescent/sched/sched.h>

#include <acpi/acpi_init.h>

static int init_status = INIT_STATUS_NOTHING;
int init_status_get(void) {
	return init_status;
}

__diag_push();
__diag_ignore("-Wmissing-prototypes");

_Noreturn __asmlinkage void kernel_main(void) {
	int base_revision = limine_base_revision();
	if (base_revision != LIMINE_BASE_REVISION)
		goto die;

	bsp_cpu_init();

	/* Attempt to load early modules */
	int err = module_load("e9hack");
	if (err && err != -ENOENT)
		printk(PRINTK_WARN "init: e9hack module init failed (is this real hardware?) err %i\n", err);

	err = tracing_init();
	if (err)
		printk(PRINTK_WARN "init: tracing_init failed with code %i\n", err);

	buddy_init();
	vmm_init();
	segments_init();
	interrupts_init();
	heap_init();

	err = cmdline_parse();
	if (err)
		printk("init: Failed to parse cmdline! err: %i\n", err);

	const char* loglevel = cmdline_get("loglevel");
	if (loglevel) {
		unsigned int level = *loglevel - '0';
		err = printk_set_level(level);
		if (err == -EINVAL)
			printk(PRINTK_WARN "init: Failed to set cmdline loglevel, invalid level: %u", level);
		else if (err)
			printk(PRINTK_WARN "init: Failed to set cmdline loglevel, err: %i", err);
	}

	init_status = INIT_STATUS_MM;

	term_init();

	err = acpi_init();
	if (err)
		panic("ACPI failed to initialize, err: %i", err);
	err = apic_bsp_init();
	if (err)
		panic("Failed to initialize APIC, err: %i", err);

	timekeeper_init();

	sched_init();
	init_status = INIT_STATUS_SCHED;

	printk(PRINTK_CRIT "init: kernel_main ended!\n");
	local_irq_enable();
die:
	while (1)
		cpu_halt();
}

__diag_pop();
