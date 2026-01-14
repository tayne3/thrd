#include "catch.hpp"
#include "thrd/thrd.h"

#define NUM_THREADS 8

static tss_t shared_tss;
static int   dtor_called;
static mtx_t dtor_mtx;

TEST_CASE("tss_create with NULL", "[input_validation]") {
	REQUIRE(tss_create(NULL, NULL) == thrd_error);
}

TEST_CASE("tss_set and tss_get", "[functional]") {
	tss_t tss;
	void *val;

	REQUIRE(tss_create(&tss, NULL) == thrd_success);

	REQUIRE(tss_set(tss, (void *)42) == thrd_success);
	val = tss_get(tss);
	REQUIRE((int)(size_t)val == 42);

	REQUIRE(tss_set(tss, (void *)100) == thrd_success);
	val = tss_get(tss);
	REQUIRE((int)(size_t)val == 100);

	tss_delete(tss);
}

static void dtor_func(void *arg) {
	mtx_lock(&dtor_mtx);
	dtor_called++;
	mtx_unlock(&dtor_mtx);
}

static int tss_thread_func(void *arg) {
	(void)arg;
	void *tss_content;

	tss_content = tss_get(shared_tss);
	REQUIRE(tss_set(shared_tss, (void *)42) == thrd_success);
	tss_content = tss_get(shared_tss);
	REQUIRE((int)(size_t)tss_content == 42);
	return 0;
}

TEST_CASE("tss destructor called", "[functional]") {
	thrd_t thread;

	REQUIRE(mtx_init(&dtor_mtx, mtx_plain) == thrd_success);
	dtor_called = 0;
	REQUIRE(tss_create(&shared_tss, dtor_func) == thrd_success);
	REQUIRE(thrd_create(&thread, tss_thread_func, NULL) == thrd_success);
	REQUIRE(thrd_join(thread, NULL) == thrd_success);

	REQUIRE(dtor_called == 1);

	tss_delete(shared_tss);
	mtx_destroy(&dtor_mtx);
}

static int isolation_thread_func(void *arg) {
	const int thread_num = (int)(size_t)arg;

	REQUIRE(tss_set(shared_tss, (void *)(size_t)(thread_num * 10)) == thrd_success);

	struct timespec dur = {0, 10000000};
	thrd_sleep(&dur, NULL);

	void *val = tss_get(shared_tss);
	REQUIRE((int)(size_t)val == thread_num * 10);
	return 0;
}

TEST_CASE("tss isolation between threads", "[functional]") {
	thrd_t threads[NUM_THREADS];

	REQUIRE(tss_create(&shared_tss, NULL) == thrd_success);

	for (int i = 0; i < NUM_THREADS; i++) {
		REQUIRE(thrd_create(threads + i, isolation_thread_func, (void *)(size_t)(i + 1)) == thrd_success);
	}
	for (int i = 0; i < NUM_THREADS; i++) {
		REQUIRE(thrd_join(threads[i], NULL) == thrd_success);
	}

	tss_delete(shared_tss);
}

TEST_CASE("tss multiple keys", "[functional]") {
	tss_t tss1, tss2;

	REQUIRE(tss_create(&tss1, NULL) == thrd_success);
	REQUIRE(tss_create(&tss2, NULL) == thrd_success);

	REQUIRE(tss_set(tss1, (void *)10) == thrd_success);
	REQUIRE(tss_set(tss2, (void *)20) == thrd_success);

	void *val1 = tss_get(tss1);
	void *val2 = tss_get(tss2);

	REQUIRE((int)(size_t)val1 == 10);
	REQUIRE((int)(size_t)val2 == 20);

	tss_delete(tss2);
	tss_delete(tss1);
}
