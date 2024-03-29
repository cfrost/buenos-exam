#include "proc/usr_semaphore.h"

/* Put your function definitions here. */
#include "kernel/interrupt.h"
#include "kernel/sleepq.h"
#include "kernel/config.h"
#include "kernel/assert.h"
#include "lib/libc.h"

/** @name Semaphores
 *
 * This module implements semaphores.
 *
 * @{
 */

/** Table containing all semaphores in the system */
static semaphore_t usr_semaphore_table[CONFIG_MAX_SEMAPHORES];

/** Lock which must be held before accessing the semaphore_table */
static spinlock_t usr_semaphore_table_slock;

/**
 * Initializes semaphore subsystem. Sets all system semaphores
 * as unreserved (non-existing).
 */

void usr_semaphore_init(void)
{
    int i;

    spinlock_reset(&usr_semaphore_table_slock);
    for(i = 0; i < CONFIG_MAX_SEMAPHORES; i++)
        usr_semaphore_table[i].creator = -1;
}

/**
 * Creates a semaphore. The actual creation is done by reserving
 * a semaphore from the semaphore table.
 *
 * @param value Initial value of the created semaphore
 *
 * @return Pointer to the created semaphore
 *
 * @see semaphore_destroy
 */

semaphore_t *usr_semaphore_create(int value)
{
    interrupt_status_t intr_status;
    static int next = 0;
    int i;
    int sem_id;

    KERNEL_ASSERT(value >= 0);

    intr_status = _interrupt_disable();
    spinlock_acquire(&usr_semaphore_table_slock);

    /* Find free semaphore from semaphore table */
    for(i = 0; i < CONFIG_MAX_SEMAPHORES; i++) {
        sem_id = next;
        next = (next + 1) % CONFIG_MAX_SEMAPHORES;
        if (usr_semaphore_table[sem_id].creator == -1) {
            usr_semaphore_table[sem_id].creator = thread_get_current_thread();
            break;
        }
    }

    spinlock_release(&usr_semaphore_table_slock);
    _interrupt_set_state(intr_status);

    if (i == CONFIG_MAX_SEMAPHORES) {
	/* semaphore table does not have any free semaphores, creation fails */
        return NULL;
    }

    usr_semaphore_table[sem_id].value = value;
    spinlock_reset(&usr_semaphore_table[sem_id].slock);

    return &usr_semaphore_table[sem_id];
}


/**
 * Decreases value of the semaphore sem by one. If semaphore has no free
 * value (its value is 0), this call will block and the call will
 * return only after the semaphores value has been increased by
 * some other thread (semaphore_V).
 *
 * The blocking is implemented by sleeping. This function
 * must not be called by interrupt handlers.
 *
 * @param sem Semaphore to lower by one.
 */

void usr_semaphore_P(semaphore_t *sem)
{
    interrupt_status_t intr_status;

    intr_status = _interrupt_disable();
    spinlock_acquire(&sem->slock);

    sem->value--;
    if (sem->value < 0) {
        sleepq_add(sem);
        spinlock_release(&sem->slock);
        thread_switch();
    } else {
        spinlock_release(&sem->slock);
    }
    _interrupt_set_state(intr_status);
}

/**
 * Increases the value of the semaphore sem by one. Wakes up
 * one waiter, if needed. 
 * 
 * Note that this function is safe to call both from interrupt handlers
 * and threads, because the call will not block.
 *
 * @param sem Semaphore to raise by one.
 *
 */ 

void usr_semaphore_V(semaphore_t *sem)
{
    interrupt_status_t intr_status;
    
    intr_status = _interrupt_disable();
    spinlock_acquire(&sem->slock);

    sem->value++;
    if (sem->value <= 0) {
        sleepq_wake(sem);
    }

    spinlock_release(&sem->slock);
    _interrupt_set_state(intr_status);
}
