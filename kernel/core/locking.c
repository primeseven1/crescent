#include <crescent/core/locking.h>
#include <crescent/core/int/irq.h>
#include <crescent/asm/flags.h>

void spinlock_lock(spinlock_t* lock) {
    while (__atomic_exchange_n(lock, 1, __ATOMIC_ACQUIRE))
        __asm__ volatile("pause" : : : "memory");
}

void spinlock_unlock(spinlock_t* lock) {
    __atomic_store_n(lock, 0, __ATOMIC_RELEASE);
}

void spinlock_lock_irq_save(spinlock_t* lock, unsigned long* flags) {
    *flags = read_cpu_flags();
    local_irq_disable();
    spinlock_lock(lock);
}

void spinlock_unlock_irq_restore(spinlock_t* lock, unsigned long* flags) {
    spinlock_unlock(lock);
    if (*flags & CPU_FLAG_INTERRUPT)
        local_irq_enable();
}
