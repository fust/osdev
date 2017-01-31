#ifndef __SPINLOCK_H
#define __SPINLOCK_H

#include "stdint.h"

typedef uint8_t volatile spinlock_t;

static void spin_lock(uint8_t volatile * lock) {
	while (__sync_lock_test_and_set(lock, 0x01));
}

static void spin_unlock(uint8_t volatile * lock) {
	__sync_lock_release(lock);
}

#endif
