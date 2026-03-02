#include "catch.hpp"
#include "thrd/thrd.h"

#define NUM_THREADS 8

static mtx_t shared_mtx;
static cnd_t shared_cnd;
static cnd_t shared_cnd2;
static int   shared_flag;

TEST_CASE("cnd_init with NULL", "[input_validation]") {
	REQUIRE(cnd_init(NULL) == thrd_error);
}

TEST_CASE("cnd_signal with NULL", "[input_validation]") {
	REQUIRE(cnd_signal(NULL) == thrd_error);
}

TEST_CASE("cnd_broadcast with NULL", "[input_validation]") {
	REQUIRE(cnd_broadcast(NULL) == thrd_error);
}

TEST_CASE("cnd_wait with NULL cnd", "[input_validation]") {
	mtx_t mtx;
	mtx_init(&mtx, mtx_plain);
	REQUIRE(cnd_wait(NULL, &mtx) == thrd_error);
	mtx_destroy(&mtx);
}

TEST_CASE("cnd_wait with NULL mtx", "[input_validation]") {
	cnd_t cnd;
	cnd_init(&cnd);
	REQUIRE(cnd_wait(&cnd, NULL) == thrd_error);
	cnd_destroy(&cnd);
}

TEST_CASE("cnd_timedwait with NULL cnd", "[input_validation]") {
	mtx_t           mtx;
	struct timespec ts;
	mtx_init(&mtx, mtx_plain);
	timespec_get(&ts, TIME_UTC);
	REQUIRE(cnd_timedwait(NULL, &mtx, &ts) == thrd_error);
	mtx_destroy(&mtx);
}

TEST_CASE("cnd_timedwait with NULL mtx", "[input_validation]") {
	cnd_t           cnd;
	struct timespec ts;
	cnd_init(&cnd);
	timespec_get(&ts, TIME_UTC);
	REQUIRE(cnd_timedwait(&cnd, NULL, &ts) == thrd_error);
	cnd_destroy(&cnd);
}

TEST_CASE("cnd_timedwait with NULL ts", "[input_validation]") {
	cnd_t cnd;
	mtx_t mtx;
	cnd_init(&cnd);
	mtx_init(&mtx, mtx_plain);
	REQUIRE(cnd_timedwait(&cnd, &mtx, NULL) == thrd_error);
	mtx_destroy(&mtx);
	cnd_destroy(&cnd);
}

static int signal_waiter_func(void *arg) {
	(void)arg;

	REQUIRE(mtx_lock(&shared_mtx) == thrd_success);
	shared_flag = 1;
	REQUIRE(cnd_signal(&shared_cnd2) == thrd_success);
	REQUIRE(cnd_wait(&shared_cnd, &shared_mtx) == thrd_success);
	shared_flag = 2;
	REQUIRE(mtx_unlock(&shared_mtx) == thrd_success);

	return 0;
}

TEST_CASE("cnd_signal", "[functional]") {
	thrd_t thread;

	REQUIRE(mtx_init(&shared_mtx, mtx_plain) == thrd_success);
	REQUIRE(cnd_init(&shared_cnd) == thrd_success);
	REQUIRE(cnd_init(&shared_cnd2) == thrd_success);
	shared_flag = 0;
	REQUIRE(thrd_create(&thread, signal_waiter_func, NULL) == thrd_success);
	REQUIRE(mtx_lock(&shared_mtx) == thrd_success);
	while (shared_flag != 1) {
		REQUIRE(cnd_wait(&shared_cnd2, &shared_mtx) == thrd_success);
	}
	REQUIRE(mtx_unlock(&shared_mtx) == thrd_success);
	REQUIRE(cnd_signal(&shared_cnd) == thrd_success);
	REQUIRE(thrd_join(thread, NULL) == thrd_success);
	REQUIRE(shared_flag == 2);

	cnd_destroy(&shared_cnd2);
	cnd_destroy(&shared_cnd);
	mtx_destroy(&shared_mtx);
}

static int broadcast_waiter_func(void *arg) {
	(void)arg;

	REQUIRE(mtx_lock(&shared_mtx) == thrd_success);
	++shared_flag;
	REQUIRE(cnd_signal(&shared_cnd2) == thrd_success);
	do {
		REQUIRE(cnd_wait(&shared_cnd, &shared_mtx) == thrd_success);
	} while (shared_flag <= NUM_THREADS);
	++shared_flag;
	REQUIRE(cnd_signal(&shared_cnd2) == thrd_success);
	REQUIRE(mtx_unlock(&shared_mtx) == thrd_success);

	return 0;
}

TEST_CASE("cnd_broadcast", "[functional]") {
	thrd_t          threads[NUM_THREADS];
	struct timespec dur;

	REQUIRE(mtx_init(&shared_mtx, mtx_plain) == thrd_success);
	REQUIRE(cnd_init(&shared_cnd) == thrd_success);
	REQUIRE(cnd_init(&shared_cnd2) == thrd_success);
	shared_flag = 0;
	dur.tv_sec  = 0;
	dur.tv_nsec = 500000000;

	for (int i = 0; i < NUM_THREADS; i++) {
		REQUIRE(thrd_create(threads + i, broadcast_waiter_func, NULL) == thrd_success);
	}

	REQUIRE(mtx_lock(&shared_mtx) == thrd_success);
	while (shared_flag != NUM_THREADS) {
		REQUIRE(cnd_wait(&shared_cnd2, &shared_mtx) == thrd_success);
	}
	REQUIRE(mtx_unlock(&shared_mtx) == thrd_success);
	REQUIRE(cnd_signal(&shared_cnd) == thrd_success);
	thrd_sleep(&dur, NULL);

	REQUIRE(mtx_lock(&shared_mtx) == thrd_success);
	shared_flag = NUM_THREADS + 1;
	REQUIRE(mtx_unlock(&shared_mtx) == thrd_success);
	REQUIRE(cnd_broadcast(&shared_cnd) == thrd_success);

	for (int i = 0; i < NUM_THREADS; i++) {
		REQUIRE(thrd_join(threads[i], NULL) == thrd_success);
	}

	cnd_destroy(&shared_cnd2);
	cnd_destroy(&shared_cnd);
	mtx_destroy(&shared_mtx);
}

TEST_CASE("cnd_timedwait timeout", "[functional]") {
	cnd_t           cnd;
	mtx_t           mtx;
	struct timespec ts;

	REQUIRE(cnd_init(&cnd) == thrd_success);
	REQUIRE(mtx_init(&mtx, mtx_plain) == thrd_success);
	REQUIRE(mtx_lock(&mtx) == thrd_success);

	REQUIRE(timespec_get(&ts, TIME_UTC) == TIME_UTC);
	ts.tv_nsec += 100000000;
	if (ts.tv_nsec >= 1000000000) {
		++ts.tv_sec;
		ts.tv_nsec -= 1000000000;
	}

	REQUIRE(cnd_timedwait(&cnd, &mtx, &ts) == thrd_timedout);

	REQUIRE(mtx_unlock(&mtx) == thrd_success);
	mtx_destroy(&mtx);
	cnd_destroy(&cnd);
}
