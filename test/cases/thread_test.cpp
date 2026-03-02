#include "catch.hpp"
#include "thrd/thrd.h"

#define NUM_THREADS 8

TEST_CASE("thrd_create with NULL thr", "[input_validation]") {
	REQUIRE(thrd_create(NULL, (thrd_start_t)1, NULL) == thrd_error);
}

TEST_CASE("thrd_create with NULL func", "[input_validation]") {
	thrd_t thr;
	REQUIRE(thrd_create(&thr, NULL, NULL) == thrd_error);
}

TEST_CASE("thrd_sleep with NULL", "[input_validation]") {
	REQUIRE(thrd_sleep(NULL, NULL) == -2);
}

static int return_value_func(void *arg) {
	return (int)(size_t)arg;
}

TEST_CASE("thrd_create and join", "[functional]") {
	thrd_t thr;
	int    res;

	REQUIRE(thrd_create(&thr, return_value_func, (void *)42) == thrd_success);
	REQUIRE(thrd_join(thr, &res) == thrd_success);
	REQUIRE(res == 42);
}

static int simple_func(void *arg) {
	(void)arg;
	return 0;
}

TEST_CASE("thrd_create multiple threads", "[functional]") {
	thrd_t threads[NUM_THREADS];

	for (int i = 0; i < NUM_THREADS; i++) {
		REQUIRE(thrd_create(threads + i, simple_func, NULL) == thrd_success);
	}
	for (int i = 0; i < NUM_THREADS; i++) {
		REQUIRE(thrd_join(threads[i], NULL) == thrd_success);
	}
}

static int detach_func(void *arg) {
	struct timespec dur;
	(void)arg;

	dur.tv_sec  = 0;
	dur.tv_nsec = 100000000; /* 100ms */
	thrd_sleep(&dur, NULL);
	return 0;
}

TEST_CASE("thrd_detach", "[functional]") {
	thrd_t          thr;
	struct timespec dur;

	REQUIRE(thrd_create(&thr, detach_func, NULL) == thrd_success);
	REQUIRE(thrd_detach(thr) == thrd_success);

	dur.tv_sec  = 0;
	dur.tv_nsec = 200000000;
	thrd_sleep(&dur, NULL);
}

TEST_CASE("thrd_current", "[functional]") {
	thrd_t current = thrd_current();
	(void)current;
}

TEST_CASE("thrd_equal", "[functional]") {
	thrd_t current = thrd_current();
	REQUIRE(thrd_equal(current, current));
}

TEST_CASE("thrd_sleep", "[functional]") {
	struct timespec start, end, dur;

	dur.tv_sec  = 0;
	dur.tv_nsec = 100000000; /* 100ms */

	timespec_get(&start, TIME_UTC);
	REQUIRE(thrd_sleep(&dur, NULL) == 0);
	timespec_get(&end, TIME_UTC);

	REQUIRE((end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec) >= 50000000L);
}

TEST_CASE("thrd_yield", "[functional]") {
	thrd_yield();
}
