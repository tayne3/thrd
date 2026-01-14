#include <stdlib.h>
#include <thrd/thrd.h>

#include "internal.h"

int mtx_init(mtx_t *mtx, int type) {
	impl_thrd_mutex_t *im;

	if (!mtx) {
		return thrd_error;
	}

	im = (impl_thrd_mutex_t *)calloc(1, sizeof(impl_thrd_mutex_t));
	if (!im) {
		return thrd_nomem;
	}

	InitializeCriticalSection(&im->cs);
	im->type = type;
	*mtx     = im;
	return thrd_success;
}

void mtx_destroy(mtx_t *mtx) {
	impl_thrd_mutex_t *im;
	if (mtx && *mtx) {
		im = (impl_thrd_mutex_t *)*mtx;
		DeleteCriticalSection(&im->cs);
		free(im);
		*mtx = NULL;
	}
}

int mtx_lock(mtx_t *mtx) {
	impl_thrd_mutex_t *im;

	if (!mtx || !*mtx) {
		return thrd_error;
	}
	im = (impl_thrd_mutex_t *)*mtx;

	EnterCriticalSection(&im->cs);
	im->count++;
	return thrd_success;
}

int mtx_trylock(mtx_t *mtx) {
	impl_thrd_mutex_t *im;

	if (!mtx || !*mtx) {
		return thrd_error;
	}
	im = (impl_thrd_mutex_t *)*mtx;

	if (TryEnterCriticalSection(&im->cs)) {
		if ((im->type & mtx_recursive) == 0 && im->count > 0) {
			LeaveCriticalSection(&im->cs);
			return thrd_busy;
		}
		im->count++;
		return thrd_success;
	}

	return thrd_busy;
}

int mtx_timedlock(mtx_t *mtx, const struct timespec *ts) {
	impl_thrd_mutex_t *im;
	DWORD              ms;
	int                clamped;

	if (!mtx || !*mtx || !ts) {
		return thrd_error;
	}
	im = (impl_thrd_mutex_t *)*mtx;

	while (1) {
		if (TryEnterCriticalSection(&im->cs)) {
			im->count++;
			return thrd_success;
		}

		ms = _thrd_win32_util_timepoint_to_ms(ts, &clamped);
		if (ms == 0 && !clamped) {
			return thrd_timedout;
		}

		Sleep(1);
	}
}

int mtx_unlock(mtx_t *mtx) {
	impl_thrd_mutex_t *im;

	if (!mtx || !*mtx) {
		return thrd_error;
	}
	im = (impl_thrd_mutex_t *)*mtx;

	im->count--;
	LeaveCriticalSection(&im->cs);
	return thrd_success;
}
