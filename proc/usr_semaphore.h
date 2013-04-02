#ifndef BUENOS_PROC_SEMAPHORE
#define BUENOS_PROC_SEMAPHORE

#include "lib/types.h"
#include "kernel/spinlock.h"
#include "kernel/thread.h"

void usr_semaphore_init(void);
semaphore_t *usr_semaphore_create(usr_sem_t *sem,int value);
void usr_semaphore_P(semaphore_t *sem);
void usr_semaphore_V(semaphore_t *sem);


/* Put your function prototypes here. */

#endif
