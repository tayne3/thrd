#include "catch.hpp"
#include "thrd/thrd.h"

#define NUM_THREADS 8

static mtx_t shared_mtx;
static mtx_t shared_mtx2;
static cnd_t shared_cnd;
static int   shared_flag;
static int   shared_counter;

TEST_CASE("mtx_init with NULL", "[input_validation]") {
	REQUIRE(mtx_init(NULL, mtx_plain) == thrd_error);
}

TEST_CASE("mtx_lock with NULL", "[input_validation]") {
	REQUIRE(mtx_lock(NULL) == thrd_error);
}

TEST_CASE("mtx_trylock with NULL", "[input_validation]") {
	REQUIRE(mtx_trylock(NULL) == thrd_error);
}

TEST_CASE("mtx_unlock with NULL", "[input_validation]") {
	REQUIRE(mtx_unlock(NULL) == thrd_error);
}

TEST_CASE("mtx_timedlock with NULL mtx", "[input_validation]") {
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	REQUIRE(mtx_timedlock(NULL, &ts) == thrd_error);
}

TEST_CASE("mtx_timedlock with NULL ts", "[input_validation]") {
	mtx_t mtx;
	mtx_init(&mtx, mtx_timed);
	REQUIRE(mtx_timedlock(&mtx, NULL) == thrd_error);
	mtx_destroy(&mtx);
}

TEST_CASE("mtx_plain lock/unlock", "[functional]") {
	mtx_t mtx;

	REQUIRE(mtx_init(&mtx, mtx_plain) == thrd_success);
	REQUIRE(mtx_lock(&mtx) == thrd_success);
	REQUIRE(mtx_unlock(&mtx) == thrd_success);
	mtx_destroy(&mtx);
}

TEST_CASE("mtx_recursive", "[functional]") {
	mtx_t mtx;

	REQUIRE(mtx_init(&mtx, mtx_plain | mtx_recursive) == thrd_success);
	REQUIRE(mtx_lock(&mtx) == thrd_success);
	REQUIRE(mtx_lock(&mtx) == thrd_success);
	REQUIRE(mtx_unlock(&mtx) == thrd_success);
	REQUIRE(mtx_unlock(&mtx) == thrd_success);
	mtx_destroy(&mtx);
}

TEST_CASE("mtx_trylock busy", "[functional]") {
	mtx_t mtx;

	REQUIRE(mtx_init(&mtx, mtx_plain) == thrd_success);
	REQUIRE(mtx_lock(&mtx) == thrd_success);
	REQUIRE(mtx_trylock(&mtx) == thrd_busy);
	REQUIRE(mtx_unlock(&mtx) == thrd_success);
	mtx_destroy(&mtx);
}

#if !defined(_WIN32) || defined(THRD_PTHREAD_WIN32) || !defined(THRD_OLD_WIN32API)
static int hold_mutex_for_one_second(void *arg) {
	struct timespec dur;
	(void)arg;

	REQUIRE(mtx_lock(&shared_mtx) == thrd_success);

	REQUIRE(mtx_lock(&shared_mtx2) == thrd_success);
	shared_flag = 1;
	REQUIRE(cnd_signal(&shared_cnd) == thrd_success);
	REQUIRE(mtx_unlock(&shared_mtx2) == thrd_success);

	dur.tv_sec  = 1;
	dur.tv_nsec = 0;
	thrd_sleep(&dur, NULL);

	REQUIRE(mtx_unlock(&shared_mtx) == thrd_success);
	return 0;
}

TEST_CASE("mtx_timedlock timeout", "[functional]") {
	thrd_t          thread;
	struct timespec ts;

	REQUIRE(mtx_init(&shared_mtx, mtx_timed) == thrd_success);
	REQUIRE(mtx_init(&shared_mtx2, mtx_plain) == thrd_success);
	REQUIRE(cnd_init(&shared_cnd) == thrd_success);
	shared_flag = 0;

	REQUIRE(thrd_create(&thread, hold_mutex_for_one_second, NULL) == thrd_success);

	REQUIRE(mtx_lock(&shared_mtx2) == thrd_success);
	while (!shared_flag) {
		REQUIRE(cnd_wait(&shared_cnd, &shared_mtx2) == thrd_success);
	}
	REQUIRE(mtx_unlock(&shared_mtx2) == thrd_success);

	REQUIRE(timespec_get(&ts, TIME_UTC) == TIME_UTC);
	ts.tv_nsec += 500000000;
	if (ts.tv_nsec >= 1000000000) {
		++ts.tv_sec;
		ts.tv_nsec -= 1000000000;
	}
	REQUIRE(mtx_timedlock(&shared_mtx, &ts) == thrd_timedout);

	REQUIRE(thrd_join(thread, NULL) == thrd_success);
	cnd_destroy(&shared_cnd);
	mtx_destroy(&shared_mtx2);
	mtx_destroy(&shared_mtx);
}

TEST_CASE("mtx_timedlock success", "[functional]") {
	mtx_t           mtx;
	struct timespec ts;

	REQUIRE(mtx_init(&mtx, mtx_timed) == thrd_success);

	REQUIRE(timespec_get(&ts, TIME_UTC) == TIME_UTC);
	ts.tv_sec += 1;
	REQUIRE(mtx_timedlock(&mtx, &ts) == thrd_success);
	REQUIRE(mtx_unlock(&mtx) == thrd_success);

	mtx_destroy(&mtx);
}
#endif

static int contention_func(void *arg) {
	(void)arg;

	for (int i = 0; i < 100; i++) {
		mtx_lock(&shared_mtx);
		shared_counter++;
		mtx_unlock(&shared_mtx);
	}
	return 0;
}

TEST_CASE("mtx contention", "[concurrency]") {
	thrd_t threads[NUM_THREADS];

	REQUIRE(mtx_init(&shared_mtx, mtx_plain) == thrd_success);
	shared_counter = 0;

	for (int i = 0; i < NUM_THREADS; i++) {
		REQUIRE(thrd_create(threads + i, contention_func, NULL) == thrd_success);
	}
	for (int i = 0; i < NUM_THREADS; i++) {
		REQUIRE(thrd_join(threads[i], NULL) == thrd_success);
	}

	REQUIRE(shared_counter == NUM_THREADS * 100);
	mtx_destroy(&shared_mtx);
}
