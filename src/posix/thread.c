#include <errno.h>
#include <sched.h>
#include <stdlib.h>
#include <thrd/thrd.h>
#include <time.h>
#include <unistd.h>

struct thrd_start_param {
	thrd_start_t func;
	void        *arg;
};

static void *thrd_start_proxy(void *p) {
	struct thrd_start_param param = *(struct thrd_start_param *)p;
	free(p);
	return (void *)(intptr_t)param.func(param.arg);
}

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg) {
	struct thrd_start_param *p;
	int                      res;

	if (!thr || !func) {
		return thrd_error;
	}

	p = (struct thrd_start_param *)malloc(sizeof(*p));
	if (!p) {
		return thrd_nomem;
	}

	p->func = func;
	p->arg  = arg;

	res = pthread_create(thr, 0, thrd_start_proxy, p);
	if (res != 0) {
		free(p);
		return res == ENOMEM ? thrd_nomem : thrd_error;
	}
	return thrd_success;
}

void thrd_exit(int res) {
	pthread_exit((void *)(intptr_t)res);
}

int thrd_join(thrd_t thr, int *res) {
	void *retval;
	if (pthread_join(thr, &retval) != 0) {
		return thrd_error;
	}
	if (res) {
		*res = (int)(intptr_t)retval;
	}
	return thrd_success;
}

int thrd_detach(thrd_t thr) {
	return pthread_detach(thr) == 0 ? thrd_success : thrd_error;
}

thrd_t thrd_current(void) {
	return pthread_self();
}

int thrd_equal(thrd_t a, thrd_t b) {
	return pthread_equal(a, b);
}

int thrd_sleep(const struct timespec *ts_in, struct timespec *rem_out) {
	if (!ts_in) {
		return -2;
	}

	if (nanosleep(ts_in, rem_out) < 0) {
		if (errno == EINTR) {
			return -1;
		}
		return -2;
	}
	return 0;
}

void thrd_yield(void) {
	sched_yield();
}
