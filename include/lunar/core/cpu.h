#pragma once

#include <lunar/asm/msr.h>
#include <lunar/sched/scheduler.h>
#include <lunar/lib/list.h>
#include <lunar/lib/ringbuffer.h>
#include <lunar/core/timekeeper.h>
#include <lunar/core/panic.h>
#include <lunar/core/semaphore.h>
#include <lunar/core/limine.h>

struct cpu {
	struct cpu* self;
	u32 processor_id, lapic_id, sched_processor_id;
	struct mm* mm_struct;
	struct runqueue runqueue;
	struct ringbuffer workqueue;
	struct semaphore workqueue_sem;
	spinlock_t workqueue_lock;
	bool need_resched;
	struct timekeeper_source* timekeeper;
};

struct smp_cpus {
	u32 count;
	struct cpu* cpus[];
};

void cpu_structs_init(void);
const struct smp_cpus* smp_cpus_get(void);
void cpu_register(void);

/**
 * @brief Get the current CPU's CPU struct
 * @return The address of the CPU struct
 */
static inline struct cpu* current_cpu(void) {
	struct cpu* cpu;
	__asm__("movq %%gs:0, %0" : "=r"(cpu));
	return cpu;
}

void cpu_init_finish(void);
void cpu_startup_aps(void);

void cpu_ap_init(struct limine_mp_info* mp_info);
void cpu_bsp_init(void);
