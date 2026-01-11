#include <stdlib.h>
#include <thrd/thrd.h>
#include <windows.h>

#include "thrd_win32_internal.h"

/*
 * Win32 Mutex Implementation Note:
 *
 * Windows CRITICAL_SECTION is always recursive by nature. This means that both
 * mtx_plain and mtx_recursive types behave identically on Windows:
 * - A thread can lock the same mutex multiple times without deadlock
 * - Each lock must be matched by an unlock
 *
 * This differs from POSIX where mtx_plain creates a non-recursive mutex that
 * would deadlock if locked twice by the same thread.
 *
 * For applications requiring strict non-recursive mutex semantics, consider
 * using a debug wrapper that tracks lock state.
 */
int mtx_init(mtx_t *mtx, int type) {
	PCRITICAL_SECTION cs;
	(void)type; /* See note above: type is ignored, always recursive */

	if (!mtx) {
		return thrd_error;
	}

	cs = (PCRITICAL_SECTION)malloc(sizeof(CRITICAL_SECTION));
	if (!cs) {
		return thrd_nomem;
	}

	InitializeCriticalSection(cs);
	*mtx = cs;
	return thrd_success;
}

void mtx_destroy(mtx_t *mtx) {
	if (mtx && *mtx) {
		DeleteCriticalSection((PCRITICAL_SECTION)*mtx);
		free(*mtx);
		*mtx = NULL;
	}
}

int mtx_lock(mtx_t *mtx) {
	if (!mtx || !*mtx) {
		return thrd_error;
	}
	EnterCriticalSection((PCRITICAL_SECTION)*mtx);
	return thrd_success;
}

int mtx_trylock(mtx_t *mtx) {
	if (!mtx || !*mtx) {
		return thrd_error;
	}
	return TryEnterCriticalSection((PCRITICAL_SECTION)*mtx) ? thrd_success : thrd_busy;
}

int mtx_timedlock(mtx_t *mtx, const struct timespec *ts) {
	int   clamped;
	DWORD ms;
	DWORD start;

	if (!mtx || !*mtx || !ts) {
		return thrd_error;
	}

	ms    = _thrd_win32_util_timepoint_to_ms(ts, &clamped);
	start = GetTickCount();

	while (1) {
		if (TryEnterCriticalSection((PCRITICAL_SECTION)*mtx)) {
			return thrd_success;
		}

		if (GetTickCount() - start >= ms) {
			return thrd_timedout;
		}

		Sleep(1);
	}
	return thrd_error;
}

int mtx_unlock(mtx_t *mtx) {
	if (!mtx || !*mtx) {
		return thrd_error;
	}
	LeaveCriticalSection((PCRITICAL_SECTION)*mtx);
	return thrd_success;
}
