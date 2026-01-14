#include <thrd/thrd.h>

int cnd_init(cnd_t *cond) {
	if (!cond) {
		return thrd_error;
	}
	return pthread_cond_init(cond, 0) == 0 ? thrd_success : thrd_error;
}

void cnd_destroy(cnd_t *cond) {
	if (cond) {
		pthread_cond_destroy(cond);
	}
}

int cnd_signal(cnd_t *cond) {
	if (!cond) {
		return thrd_error;
	}
	return pthread_cond_signal(cond) == 0 ? thrd_success : thrd_error;
}

int cnd_broadcast(cnd_t *cond) {
	if (!cond) {
		return thrd_error;
	}
	return pthread_cond_broadcast(cond) == 0 ? thrd_success : thrd_error;
}

int cnd_wait(cnd_t *cond, mtx_t *mtx) {
	if (!cond || !mtx) {
		return thrd_error;
	}
	return pthread_cond_wait(cond, mtx) == 0 ? thrd_success : thrd_error;
}

int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts) {
	int res;
	if (!cond || !mtx || !ts) {
		return thrd_error;
	}
	if ((res = pthread_cond_timedwait(cond, mtx, ts)) != 0) {
		return res == ETIMEDOUT ? thrd_timedout : thrd_error;
	}
	return thrd_success;
}
