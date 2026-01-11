#ifndef _WIN32

#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <thrd/thrd.h>
#include <time.h>

#ifdef __APPLE__
/* Darwin doesn't implement timed mutexes currently */
#define THRD_NO_TIMED_MUTEX
#endif

#ifdef THRD_NO_TIMED_MUTEX
#define THRD_TIMEDLOCK_POLL_INTERVAL 5000000 /* 5 ms */
#endif

int mtx_init(mtx_t *mtx, int type) {
	int                 res;
	pthread_mutexattr_t attr;

	if (!mtx)
		return thrd_error;

	pthread_mutexattr_init(&attr);

	if (type & mtx_timed) {
#ifdef PTHREAD_MUTEX_TIMED_NP
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_TIMED_NP);
#else
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
#endif
	}
	if (type & mtx_recursive) {
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	}

	res = pthread_mutex_init(mtx, &attr) == 0 ? thrd_success : thrd_error;
	pthread_mutexattr_destroy(&attr);
	return res;
}

void mtx_destroy(mtx_t *mtx) {
	if (mtx)
		pthread_mutex_destroy(mtx);
}

int mtx_lock(mtx_t *mtx) {
	if (!mtx)
		return thrd_error;
	return pthread_mutex_lock(mtx) == 0 ? thrd_success : thrd_error;
}

int mtx_trylock(mtx_t *mtx) {
	int res;

	if (!mtx)
		return thrd_error;

	res = pthread_mutex_trylock(mtx);
	if (res == EBUSY) {
		return thrd_busy;
	}
	return res == 0 ? thrd_success : thrd_error;
}

int mtx_timedlock(mtx_t *mtx, const struct timespec *ts) {
	int res = 0;

	if (!mtx || !ts)
		return thrd_error;

#ifdef THRD_NO_TIMED_MUTEX
	/* fake a timedlock by polling trylock in a loop */
	struct timeval  now;
	struct timespec sleeptime;

	sleeptime.tv_sec  = 0;
	sleeptime.tv_nsec = THRD_TIMEDLOCK_POLL_INTERVAL;

	while ((res = pthread_mutex_trylock(mtx)) == EBUSY) {
		gettimeofday(&now, NULL);

		if (now.tv_sec > ts->tv_sec || (now.tv_sec == ts->tv_sec && (now.tv_usec * 1000) >= ts->tv_nsec)) {
			return thrd_timedout;
		}

		nanosleep(&sleeptime, NULL);
	}
#else
	if ((res = pthread_mutex_timedlock(mtx, ts)) == ETIMEDOUT) {
		return thrd_timedout;
	}
#endif
	return res == 0 ? thrd_success : thrd_error;
}

int mtx_unlock(mtx_t *mtx) {
	if (!mtx)
		return thrd_error;
	return pthread_mutex_unlock(mtx) == 0 ? thrd_success : thrd_error;
}

#endif /* !_WIN32 */
