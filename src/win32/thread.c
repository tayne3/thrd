#include <stdlib.h>
#include <thrd/thrd.h>

#define WIN32_LEAN_AND_MEAN
#include <process.h> /* for _beginthreadex */
#include <windows.h>

#include "thrd_win32_internal.h"

struct _thrd_start_param {
	thrd_start_t func;
	void        *arg;
};

static unsigned __stdcall _thrd_start_proxy(void *p) {
	struct _thrd_start_param param = *(struct _thrd_start_param *)p;
	free(p);
	return (unsigned)param.func(param.arg);
}

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg) {
	struct _thrd_start_param *p;
	uintptr_t                 handle;
	unsigned                  id;

	if (!thr || !func) {
		return thrd_error;
	}

	p = (struct _thrd_start_param *)malloc(sizeof(*p));
	if (!p) {
		return thrd_nomem;
	}
	p->func = func;
	p->arg  = arg;

	handle = _beginthreadex(NULL, 0, _thrd_start_proxy, p, 0, &id);
	if (handle == 0) {
		free(p);
		return thrd_error;
	}

	thr->handle = (void *)handle;
	thr->id     = id;
	return thrd_success;
}

void thrd_exit(int res) {
	_thrd_win32_tss_cleanup();
	_endthreadex((unsigned)res);
}

int thrd_join(thrd_t thr, int *res) {
	DWORD wres;
	DWORD exit_code;

	if (!thr.handle) {
		return thrd_error;
	}

	wres = WaitForSingleObject(thr.handle, INFINITE);
	if (wres != WAIT_OBJECT_0) {
		return thrd_error;
	}

	if (res) {
		if (GetExitCodeThread(thr.handle, &exit_code)) {
			*res = (int)exit_code;
		} else {
			return thrd_error;
		}
	}
	CloseHandle(thr.handle);
	return thrd_success;
}

int thrd_detach(thrd_t thr) {
	if (!thr.handle) {
		return thrd_error;
	}
	return CloseHandle(thr.handle) ? thrd_success : thrd_error;
}

thrd_t thrd_current(void) {
	thrd_t t;
	t.handle = NULL;
	t.id     = GetCurrentThreadId();
	return t;
}

int thrd_equal(thrd_t a, thrd_t b) {
	return a.id == b.id;
}

void thrd_yield(void) {
	SwitchToThread();
}

int thrd_sleep(const struct timespec *ts_in, struct timespec *rem_out) {
	DWORD         ms;
	LARGE_INTEGER freq, start, end;
	long long     req_ns, elapsed_ns, remaining_ns;

	if (!_thrd_win32_timespec_to_ms(ts_in, &ms)) {
		return -2;
	}

	/* Get start time for remaining calculation */
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);

	Sleep(ms);

	if (rem_out) {
		QueryPerformanceCounter(&end);

		/* Calculate requested time in nanoseconds */
		req_ns = (long long)ts_in->tv_sec * 1000000000LL + ts_in->tv_nsec;

		/* Calculate elapsed time in nanoseconds */
		elapsed_ns = (end.QuadPart - start.QuadPart) * 1000000000LL / freq.QuadPart;

		/* Calculate remaining time */
		remaining_ns = req_ns - elapsed_ns;
		if (remaining_ns < 0)
			remaining_ns = 0;

		rem_out->tv_sec  = (long)(remaining_ns / 1000000000LL);
		rem_out->tv_nsec = (long)(remaining_ns % 1000000000LL);
	}

	return 0;
}
