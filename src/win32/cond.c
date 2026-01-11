#include <stdlib.h>
#include <thrd/thrd.h>
#include <windows.h>

#include "internal.h"

int cnd_init(cnd_t *cond) {
	PCONDITION_VARIABLE cv;

	if (!cond) {
		return thrd_error;
	}

	cv = (PCONDITION_VARIABLE)malloc(sizeof(CONDITION_VARIABLE));
	if (!cv) {
		return thrd_nomem;
	}

	InitializeConditionVariable(cv);
	*cond = cv;
	return thrd_success;
}

void cnd_destroy(cnd_t *cond) {
	if (cond && *cond) {
		free(*cond);
		*cond = NULL;
	}
}

int cnd_signal(cnd_t *cond) {
	if (!cond || !*cond) {
		return thrd_error;
	}
	WakeConditionVariable((PCONDITION_VARIABLE)*cond);
	return thrd_success;
}

int cnd_broadcast(cnd_t *cond) {
	if (!cond || !*cond) {
		return thrd_error;
	}
	WakeAllConditionVariable((PCONDITION_VARIABLE)*cond);
	return thrd_success;
}

int cnd_wait(cnd_t *cond, mtx_t *mtx) {
	if (!cond || !*cond || !mtx || !*mtx) {
		return thrd_error;
	}
	if (SleepConditionVariableCS((PCONDITION_VARIABLE)*cond, (PCRITICAL_SECTION)*mtx, INFINITE)) {
		return thrd_success;
	}
	return thrd_error;
}

int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts) {
	int   clamped;
	DWORD ms;

	if (!cond || !*cond || !mtx || !*mtx || !ts) {
		return thrd_error;
	}

	ms = _thrd_win32_util_timepoint_to_ms(ts, &clamped);

	if (SleepConditionVariableCS((PCONDITION_VARIABLE)*cond, (PCRITICAL_SECTION)*mtx, ms)) {
		return thrd_success;
	}

	return (GetLastError() == ERROR_TIMEOUT) ? thrd_timedout : thrd_error;
}
