#pragma once

typedef volatile unsigned long spinlock_t;

#define SPINLOCK_INITIALIZER 0

/**
 * @brief Acquire a spinlock
 *
 * This function does busy waiting until the lock is free. Improper use can
 * potentially lead to deadlocks.
 *
 * @param lock The lock to acquire
 */
void spinlock_lock(spinlock_t* lock);

/**
 * @brief Release a spinlock
 * @param lock The lock to release
 */
void spinlock_unlock(spinlock_t* lock);

/**
 * @brief Acquire a spinlock, save the IRQ state and disable interrupts
 * @param lock The lock to acquire
 * @param flags The pointer to a variable on the stack for the lock flags
 */
void spinlock_lock_irq_save(spinlock_t* lock, unsigned long* flags);

/**
 * @brief Release a spinlock, and restore the IRQ state
 * @param lock The lock to release
 * @param flags The pointer to the variable on the stack for the lock flags
 */
void spinlock_unlock_irq_restore(spinlock_t* lock, unsigned long* flags);
