#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "cunit.h"
#include "thrd/thrd.h"

#define NUM_THREADS 8

static mtx_t shared_mtx;
static mtx_t shared_mtx2;
static cnd_t shared_cnd;
static int   shared_flag;
static int   shared_counter;

void test_mtx_init_null(void) {
	assert_int_eq(thrd_error, mtx_init(NULL, mtx_plain));
}

void test_mtx_lock_null(void) {
	assert_int_eq(thrd_error, mtx_lock(NULL));
}

#ifdef THRD_USE_WIN32
void test_mtx_lock_null_ptr(void) {
	mtx_t mtx = NULL;
	assert_int_eq(thrd_error, mtx_lock(&mtx));
}
#endif

void test_mtx_trylock_null(void) {
	assert_int_eq(thrd_error, mtx_trylock(NULL));
}

void test_mtx_unlock_null(void) {
	assert_int_eq(thrd_error, mtx_unlock(NULL));
}

void test_mtx_timedlock_null_mtx(void) {
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	assert_int_eq(thrd_error, mtx_timedlock(NULL, &ts));
}

void test_mtx_timedlock_null_ts(void) {
	mtx_t mtx;
	mtx_init(&mtx, mtx_timed);
	assert_int_eq(thrd_error, mtx_timedlock(&mtx, NULL));
	mtx_destroy(&mtx);
}

void test_mtx_plain(void) {
	mtx_t mtx;

	assert_int_eq(thrd_success, mtx_init(&mtx, mtx_plain));
	assert_int_eq(thrd_success, mtx_lock(&mtx));
	assert_int_eq(thrd_success, mtx_unlock(&mtx));
	mtx_destroy(&mtx);
}

void test_mtx_recursive(void) {
	mtx_t mtx;

	assert_int_eq(thrd_success, mtx_init(&mtx, mtx_plain | mtx_recursive));
	assert_int_eq(thrd_success, mtx_lock(&mtx));
	assert_int_eq(thrd_success, mtx_lock(&mtx));
	assert_int_eq(thrd_success, mtx_unlock(&mtx));
	assert_int_eq(thrd_success, mtx_unlock(&mtx));
	mtx_destroy(&mtx);
}

void test_mtx_trylock_busy(void) {
	mtx_t mtx;

	assert_int_eq(thrd_success, mtx_init(&mtx, mtx_plain));
	assert_int_eq(thrd_success, mtx_lock(&mtx));
	assert_int_eq(thrd_busy, mtx_trylock(&mtx));
	assert_int_eq(thrd_success, mtx_unlock(&mtx));
	mtx_destroy(&mtx);
}

#if !defined(_WIN32) || defined(THRD_PTHREAD_WIN32) || !defined(THRD_OLD_WIN32API)
static int hold_mutex_for_one_second(void *arg) {
	struct timespec dur;
	(void)arg;

	assert_int_eq(thrd_success, mtx_lock(&shared_mtx));

	assert_int_eq(thrd_success, mtx_lock(&shared_mtx2));
	shared_flag = 1;
	assert_int_eq(thrd_success, cnd_signal(&shared_cnd));
	assert_int_eq(thrd_success, mtx_unlock(&shared_mtx2));

	dur.tv_sec  = 1;
	dur.tv_nsec = 0;
	thrd_sleep(&dur, NULL);

	assert_int_eq(thrd_success, mtx_unlock(&shared_mtx));
	return 0;
}

void test_mtx_timedlock_timeout(void) {
	thrd_t          thread;
	struct timespec ts;

	assert_int_eq(thrd_success, mtx_init(&shared_mtx, mtx_timed));
	assert_int_eq(thrd_success, mtx_init(&shared_mtx2, mtx_plain));
	assert_int_eq(thrd_success, cnd_init(&shared_cnd));
	shared_flag = 0;

	assert_int_eq(thrd_success, thrd_create(&thread, hold_mutex_for_one_second, NULL));

	assert_int_eq(thrd_success, mtx_lock(&shared_mtx2));
	while (!shared_flag) {
		assert_int_eq(thrd_success, cnd_wait(&shared_cnd, &shared_mtx2));
	}
	assert_int_eq(thrd_success, mtx_unlock(&shared_mtx2));

	assert_int_eq(TIME_UTC, timespec_get(&ts, TIME_UTC));
	ts.tv_nsec += 500000000;
	if (ts.tv_nsec >= 1000000000) {
		++ts.tv_sec;
		ts.tv_nsec -= 1000000000;
	}
	assert_int_eq(thrd_timedout, mtx_timedlock(&shared_mtx, &ts));

	assert_int_eq(thrd_success, thrd_join(thread, NULL));
	cnd_destroy(&shared_cnd);
	mtx_destroy(&shared_mtx2);
	mtx_destroy(&shared_mtx);
}

void test_mtx_timedlock_success(void) {
	mtx_t           mtx;
	struct timespec ts;

	assert_int_eq(thrd_success, mtx_init(&mtx, mtx_timed));

	assert_int_eq(TIME_UTC, timespec_get(&ts, TIME_UTC));
	ts.tv_sec += 1;
	assert_int_eq(thrd_success, mtx_timedlock(&mtx, &ts));
	assert_int_eq(thrd_success, mtx_unlock(&mtx));

	mtx_destroy(&mtx);
}
#endif

static int contention_func(void *arg) {
	int i;
	(void)arg;

	for (i = 0; i < 100; i++) {
		mtx_lock(&shared_mtx);
		shared_counter++;
		mtx_unlock(&shared_mtx);
	}
	return 0;
}

void test_mtx_contention(void) {
	int    i;
	thrd_t threads[NUM_THREADS];

	assert_int_eq(thrd_success, mtx_init(&shared_mtx, mtx_plain));
	shared_counter = 0;

	for (i = 0; i < NUM_THREADS; i++) {
		assert_int_eq(thrd_success, thrd_create(threads + i, contention_func, NULL));
	}
	for (i = 0; i < NUM_THREADS; i++) {
		assert_int_eq(thrd_success, thrd_join(threads[i], NULL));
	}

	assert_int_eq(NUM_THREADS * 100, shared_counter);
	mtx_destroy(&shared_mtx);
}

int main(void) {
	cunit_init();

	CUNIT_SUITE_BEGIN("Input Validation", NULL, NULL)
	CUNIT_TEST("mtx_init with NULL", test_mtx_init_null)
	CUNIT_TEST("mtx_lock with NULL", test_mtx_lock_null)
#ifdef THRD_USE_WIN32
	CUNIT_TEST("mtx_lock with NULL pointer", test_mtx_lock_null_ptr)
#endif
	CUNIT_TEST("mtx_trylock with NULL", test_mtx_trylock_null)
	CUNIT_TEST("mtx_unlock with NULL", test_mtx_unlock_null)
	CUNIT_TEST("mtx_timedlock with NULL mtx", test_mtx_timedlock_null_mtx)
	CUNIT_TEST("mtx_timedlock with NULL ts", test_mtx_timedlock_null_ts)
	CUNIT_SUITE_END()

	CUNIT_SUITE_BEGIN("Functional", NULL, NULL)
	CUNIT_TEST("mtx_plain lock/unlock", test_mtx_plain)
	CUNIT_TEST("mtx_recursive", test_mtx_recursive)
	CUNIT_TEST("mtx_trylock busy", test_mtx_trylock_busy)
#if !defined(_WIN32) || defined(THRD_PTHREAD_WIN32) || !defined(THRD_OLD_WIN32API)
	CUNIT_TEST("mtx_timedlock timeout", test_mtx_timedlock_timeout)
	CUNIT_TEST("mtx_timedlock success", test_mtx_timedlock_success)
#endif
	CUNIT_SUITE_END()

	CUNIT_SUITE_BEGIN("Concurrency", NULL, NULL)
	CUNIT_TEST("mtx contention", test_mtx_contention)
	CUNIT_SUITE_END()

	const int ret = cunit_run();
#ifdef _MSC_VER
	if (_CrtDumpMemoryLeaks()) {
		abort();
	}
#endif
	return ret;
}
