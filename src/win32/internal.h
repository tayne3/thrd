#ifndef THRD_WIN32_INTERNAL_H
#define THRD_WIN32_INTERNAL_H

#include <thrd/thrd.h>
#include <windows.h>

int   _thrd_win32_timespec_to_ms(const struct timespec *ts, DWORD *ms);
DWORD _thrd_win32_util_timepoint_to_ms(const struct timespec *ts_abs, int *clamped);

/* TSS cleanup - must be called before thread exit to trigger destructors */
void _thrd_win32_tss_cleanup(void);

/* TSS key registration - called by tss_create to track keys for cleanup */
void _thrd_win32_tss_register(tss_t key, tss_dtor_t dtor);
void _thrd_win32_tss_unregister(tss_t key);

typedef struct {
	CRITICAL_SECTION cs;
	int              type;
	int              count;
} impl_thrd_mutex_t;

#endif
