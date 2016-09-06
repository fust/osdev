#ifndef __SPINLOCK_H
#define __SPINLOCK_H

static void spin_lock(int volatile * lock) {
	while (!__sync_bool_compare_and_swap(lock, 0, 1));
}

static void spin_unlock(int volatile * lock) {
	__sync_lock_release(lock);
}

#endif
