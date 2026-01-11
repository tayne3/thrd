#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "cunit.h"
#include "thrd/thrd.h"

#define NUM_THREADS 8

static mtx_t shared_mtx;
static cnd_t shared_cnd;
static cnd_t shared_cnd2;
static int   shared_flag;

void test_cnd_init_null(void) {
	assert_int_eq(thrd_error, cnd_init(NULL));
}

void test_cnd_signal_null(void) {
	assert_int_eq(thrd_error, cnd_signal(NULL));
}

#ifdef THRD_USE_WIN32
void test_cnd_signal_null_ptr(void) {
	cnd_t cnd = NULL;
	assert_int_eq(thrd_error, cnd_signal(&cnd));
}
#endif

void test_cnd_broadcast_null(void) {
	assert_int_eq(thrd_error, cnd_broadcast(NULL));
}

void test_cnd_wait_null_cnd(void) {
	mtx_t mtx;
	mtx_init(&mtx, mtx_plain);
	assert_int_eq(thrd_error, cnd_wait(NULL, &mtx));
	mtx_destroy(&mtx);
}

void test_cnd_wait_null_mtx(void) {
	cnd_t cnd;
	cnd_init(&cnd);
	assert_int_eq(thrd_error, cnd_wait(&cnd, NULL));
	cnd_destroy(&cnd);
}

void test_cnd_timedwait_null_cnd(void) {
	mtx_t           mtx;
	struct timespec ts;
	mtx_init(&mtx, mtx_plain);
	timespec_get(&ts, TIME_UTC);
	assert_int_eq(thrd_error, cnd_timedwait(NULL, &mtx, &ts));
	mtx_destroy(&mtx);
}

void test_cnd_timedwait_null_mtx(void) {
	cnd_t           cnd;
	struct timespec ts;
	cnd_init(&cnd);
	timespec_get(&ts, TIME_UTC);
	assert_int_eq(thrd_error, cnd_timedwait(&cnd, NULL, &ts));
	cnd_destroy(&cnd);
}

void test_cnd_timedwait_null_ts(void) {
	cnd_t cnd;
	mtx_t mtx;
	cnd_init(&cnd);
	mtx_init(&mtx, mtx_plain);
	assert_int_eq(thrd_error, cnd_timedwait(&cnd, &mtx, NULL));
	mtx_destroy(&mtx);
	cnd_destroy(&cnd);
}

static int signal_waiter_func(void *arg) {
	(void)arg;

	assert_int_eq(thrd_success, mtx_lock(&shared_mtx));
	shared_flag = 1;
	assert_int_eq(thrd_success, cnd_signal(&shared_cnd2));
	assert_int_eq(thrd_success, cnd_wait(&shared_cnd, &shared_mtx));
	shared_flag = 2;
	assert_int_eq(thrd_success, mtx_unlock(&shared_mtx));

	return 0;
}

void test_cnd_signal(void) {
	thrd_t thread;

	assert_int_eq(thrd_success, mtx_init(&shared_mtx, mtx_plain));
	assert_int_eq(thrd_success, cnd_init(&shared_cnd));
	assert_int_eq(thrd_success, cnd_init(&shared_cnd2));
	shared_flag = 0;
	assert_int_eq(thrd_success, thrd_create(&thread, signal_waiter_func, NULL));
	assert_int_eq(thrd_success, mtx_lock(&shared_mtx));
	while (shared_flag != 1) {
		assert_int_eq(thrd_success, cnd_wait(&shared_cnd2, &shared_mtx));
	}
	assert_int_eq(thrd_success, mtx_unlock(&shared_mtx));
	assert_int_eq(thrd_success, cnd_signal(&shared_cnd));
	assert_int_eq(thrd_success, thrd_join(thread, NULL));
	assert_int_eq(2, shared_flag);

	cnd_destroy(&shared_cnd2);
	cnd_destroy(&shared_cnd);
	mtx_destroy(&shared_mtx);
}

static int broadcast_waiter_func(void *arg) {
	(void)arg;

	assert_int_eq(thrd_success, mtx_lock(&shared_mtx));
	++shared_flag;
	assert_int_eq(thrd_success, cnd_signal(&shared_cnd2));
	do {
		assert_int_eq(thrd_success, cnd_wait(&shared_cnd, &shared_mtx));
	} while (shared_flag <= NUM_THREADS);
	++shared_flag;
	assert_int_eq(thrd_success, cnd_signal(&shared_cnd2));
	assert_int_eq(thrd_success, mtx_unlock(&shared_mtx));

	return 0;
}

void test_cnd_broadcast(void) {
	int             i;
	thrd_t          threads[NUM_THREADS];
	struct timespec dur;

	assert_int_eq(thrd_success, mtx_init(&shared_mtx, mtx_plain));
	assert_int_eq(thrd_success, cnd_init(&shared_cnd));
	assert_int_eq(thrd_success, cnd_init(&shared_cnd2));
	shared_flag = 0;
	dur.tv_sec  = 0;
	dur.tv_nsec = 500000000;

	for (i = 0; i < NUM_THREADS; i++) {
		assert_int_eq(thrd_success, thrd_create(threads + i, broadcast_waiter_func, NULL));
	}

	assert_int_eq(thrd_success, mtx_lock(&shared_mtx));
	while (shared_flag != NUM_THREADS) {
		assert_int_eq(thrd_success, cnd_wait(&shared_cnd2, &shared_mtx));
	}
	assert_int_eq(thrd_success, mtx_unlock(&shared_mtx));
	assert_int_eq(thrd_success, cnd_signal(&shared_cnd));
	thrd_sleep(&dur, NULL);

	assert_int_eq(thrd_success, mtx_lock(&shared_mtx));
	shared_flag = NUM_THREADS + 1;
	assert_int_eq(thrd_success, mtx_unlock(&shared_mtx));
	assert_int_eq(thrd_success, cnd_broadcast(&shared_cnd));

	for (i = 0; i < NUM_THREADS; i++) {
		assert_int_eq(thrd_success, thrd_join(threads[i], NULL));
	}

	cnd_destroy(&shared_cnd2);
	cnd_destroy(&shared_cnd);
	mtx_destroy(&shared_mtx);
}

void test_cnd_timedwait_timeout(void) {
	cnd_t           cnd;
	mtx_t           mtx;
	struct timespec ts;

	assert_int_eq(thrd_success, cnd_init(&cnd));
	assert_int_eq(thrd_success, mtx_init(&mtx, mtx_plain));
	assert_int_eq(thrd_success, mtx_lock(&mtx));

	assert_int_eq(TIME_UTC, timespec_get(&ts, TIME_UTC));
	ts.tv_nsec += 100000000;
	if (ts.tv_nsec >= 1000000000) {
		++ts.tv_sec;
		ts.tv_nsec -= 1000000000;
	}

	assert_int_eq(thrd_timedout, cnd_timedwait(&cnd, &mtx, &ts));

	assert_int_eq(thrd_success, mtx_unlock(&mtx));
	mtx_destroy(&mtx);
	cnd_destroy(&cnd);
}

int main(void) {
	cunit_init();

	CUNIT_SUITE_BEGIN("Input Validation", NULL, NULL)
	CUNIT_TEST("cnd_init with NULL", test_cnd_init_null)
	CUNIT_TEST("cnd_signal with NULL", test_cnd_signal_null)
#ifdef THRD_USE_WIN32
	CUNIT_TEST("cnd_signal with NULL pointer", test_cnd_signal_null_ptr)
#endif
	CUNIT_TEST("cnd_broadcast with NULL", test_cnd_broadcast_null)
	CUNIT_TEST("cnd_wait with NULL cnd", test_cnd_wait_null_cnd)
	CUNIT_TEST("cnd_wait with NULL mtx", test_cnd_wait_null_mtx)
	CUNIT_TEST("cnd_timedwait with NULL cnd", test_cnd_timedwait_null_cnd)
	CUNIT_TEST("cnd_timedwait with NULL mtx", test_cnd_timedwait_null_mtx)
	CUNIT_TEST("cnd_timedwait with NULL ts", test_cnd_timedwait_null_ts)
	CUNIT_SUITE_END()

	CUNIT_SUITE_BEGIN("Functional", NULL, NULL)
	CUNIT_TEST("cnd_signal", test_cnd_signal)
	CUNIT_TEST("cnd_broadcast", test_cnd_broadcast)
	CUNIT_TEST("cnd_timedwait timeout", test_cnd_timedwait_timeout)
	CUNIT_SUITE_END()

	const int ret = cunit_run();
#ifdef _WIN32
	if (_CrtDumpMemoryLeaks()) {
		abort();
	}
#endif
	return ret;
}
