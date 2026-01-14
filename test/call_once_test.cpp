#include "catch.hpp"
#include "thrd/thrd.h"

#define NUM_THREADS 8

static once_flag shared_once = ONCE_FLAG_INIT;
static int       shared_flag;

TEST_CASE("call_once with both NULL", "[input_validation]") {
	call_once(NULL, NULL);
}

static once_flag single_thread_once = ONCE_FLAG_INIT;
static int       single_thread_counter;

static void single_thread_func(void) {
	++single_thread_counter;
}

TEST_CASE("call_once single thread", "[functional]") {
	single_thread_counter = 0;

	call_once(&single_thread_once, single_thread_func);
	call_once(&single_thread_once, single_thread_func);
	call_once(&single_thread_once, single_thread_func);

	REQUIRE(single_thread_counter == 1);
}

static void call_once_func(void) {
	++shared_flag;
}

static int call_once_thread_func(void *arg) {
	(void)arg;
	call_once(&shared_once, call_once_func);
	return 0;
}

TEST_CASE("call_once multiple threads", "[concurrency]") {
	thrd_t threads[NUM_THREADS];

	shared_flag = 0;

	for (int i = 0; i < NUM_THREADS; i++) {
		REQUIRE(thrd_create(threads + i, call_once_thread_func, NULL) == thrd_success);
	}
	for (int i = 0; i < NUM_THREADS; i++) {
		REQUIRE(thrd_join(threads[i], NULL) == thrd_success);
	}
	REQUIRE(shared_flag == 1);
}
