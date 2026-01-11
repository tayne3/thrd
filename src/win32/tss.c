#include <stdlib.h>
#include <thrd/thrd.h>
#include <windows.h>

#include "internal.h"

/*
 * TSS Key Registry
 *
 * We maintain a simple linked list of TSS keys and their destructors.
 * This allows thrd_exit() to properly invoke destructors, which
 * _endthreadex() alone does not do.
 *
 * Note: FLS callbacks are still used as the primary destructor mechanism
 * for normal thread return. This registry is specifically for thrd_exit().
 */
struct tss_entry {
	tss_t             key;
	tss_dtor_t        dtor;
	struct tss_entry *next;
};

static struct tss_entry *tss_registry = NULL;
static CRITICAL_SECTION  tss_registry_lock;
static LONG              tss_registry_init = 0;

static void tss_registry_ensure_init(void) {
	if (InterlockedCompareExchange(&tss_registry_init, 1, 0) == 0) {
		InitializeCriticalSection(&tss_registry_lock);
	}
}

void _thrd_win32_tss_register(tss_t key, tss_dtor_t dtor) {
	struct tss_entry *entry;

	if (!dtor)
		return; /* No destructor, no need to track */

	tss_registry_ensure_init();

	entry = (struct tss_entry *)malloc(sizeof(*entry));
	if (!entry)
		return;

	entry->key  = key;
	entry->dtor = dtor;

	EnterCriticalSection(&tss_registry_lock);
	entry->next  = tss_registry;
	tss_registry = entry;
	LeaveCriticalSection(&tss_registry_lock);
}

void _thrd_win32_tss_unregister(tss_t key) {
	struct tss_entry **pp;
	struct tss_entry  *entry;

	tss_registry_ensure_init();

	EnterCriticalSection(&tss_registry_lock);
	for (pp = &tss_registry; *pp; pp = &(*pp)->next) {
		if ((*pp)->key == key) {
			entry = *pp;
			*pp   = entry->next;
			free(entry);
			break;
		}
	}
	LeaveCriticalSection(&tss_registry_lock);
}

void _thrd_win32_tss_cleanup(void) {
	int iterations;

	tss_registry_ensure_init();

	/*
	 * C11 requires up to TSS_DTOR_ITERATIONS attempts to clear TSS.
	 * A destructor may set new TSS values, so we loop until no values remain
	 * or we hit the iteration limit.
	 */
	for (iterations = 0; iterations < TSS_DTOR_ITERATIONS; iterations++) {
		struct tss_entry *entry;
		int               had_value = 0;

		EnterCriticalSection(&tss_registry_lock);
		for (entry = tss_registry; entry; entry = entry->next) {
			void *val = FlsGetValue(entry->key);
			if (val) {
				had_value = 1;
				FlsSetValue(entry->key, NULL);
				LeaveCriticalSection(&tss_registry_lock);
				entry->dtor(val);
				EnterCriticalSection(&tss_registry_lock);
			}
		}
		LeaveCriticalSection(&tss_registry_lock);

		if (!had_value)
			break;
	}
}

int tss_create(tss_t *key, tss_dtor_t dtor) {
	if (!key) {
		return thrd_error;
	}
	*key = FlsAlloc((PFLS_CALLBACK_FUNCTION)dtor);
	if (*key == FLS_OUT_OF_INDEXES) {
		return thrd_error;
	}

	/* Register for thrd_exit cleanup */
	_thrd_win32_tss_register(*key, dtor);
	return thrd_success;
}

void tss_delete(tss_t key) {
	_thrd_win32_tss_unregister(key);
	FlsFree(key);
}

int tss_set(tss_t key, void *val) {
	if (key == FLS_OUT_OF_INDEXES) {
		return thrd_error;
	}
	return FlsSetValue(key, val) ? thrd_success : thrd_error;
}

void *tss_get(tss_t key) {
	return FlsGetValue(key);
}
