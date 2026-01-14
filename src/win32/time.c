#include <limits.h>
#include <thrd/thrd.h>
#include <windows.h>

#include "internal.h"

int _thrd_win32_timespec_to_ms(const struct timespec *ts, DWORD *ms) {
	if (!ts) {
		return 0;
	}
	if (ts->tv_sec < 0 || ts->tv_nsec < 0 || ts->tv_nsec >= 1000000000) {
		return 0;
	}
	if (ts->tv_sec > INT_MAX / 1000) {
		*ms = INFINITE;
		return 1;
	}
	*ms = (DWORD)ts->tv_sec * 1000 + (DWORD)(ts->tv_nsec + 999999) / 1000000;
	return 1;
}

DWORD _thrd_win32_util_timepoint_to_ms(const struct timespec *ts_abs, int *clamped) {
	struct timespec now;
	struct timespec diff;
	DWORD           ms;

	if (!ts_abs) {
		return 0;
	}

	*clamped = 0;
	timespec_get(&now, TIME_UTC);

	if (ts_abs->tv_sec < now.tv_sec || (ts_abs->tv_sec == now.tv_sec && ts_abs->tv_nsec <= now.tv_nsec)) {
		return 0;  // Already passed
	}

	diff.tv_sec  = ts_abs->tv_sec - now.tv_sec;
	diff.tv_nsec = ts_abs->tv_nsec - now.tv_nsec;
	if (diff.tv_nsec < 0) {
		diff.tv_sec--;
		diff.tv_nsec += 1000000000;
	}

	if (!_thrd_win32_timespec_to_ms(&diff, &ms)) {
		*clamped = 1;
		return INFINITE - 1;
	}

	if (ms == INFINITE) {
		*clamped = 1;
	}

	return ms;
}

#ifndef _UCRT
int timespec_get(struct timespec *ts, int base) {
	FILETIME       ft;
	ULARGE_INTEGER li;

	if (!ts) {
		return 0;
	}
	if (base != TIME_UTC) {
		return 0;
	}

	GetSystemTimeAsFileTime(&ft);
	li.LowPart  = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	ts->tv_sec  = (long long)(li.QuadPart / 10000000ULL - 11644473600ULL);
	ts->tv_nsec = (long)((li.QuadPart % 10000000ULL) * 100);

	return base;
}
#endif
