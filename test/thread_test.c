#include "cunit.h"
#include "thrd/thrd.h"

#define NUM_THREADS 8

void test_thrd_create_null_thr(void) {
	assert_int_eq(thrd_error, thrd_create(NULL, (thrd_start_t)1, NULL));
}

void test_thrd_create_null_func(void) {
	thrd_t thr;
	assert_int_eq(thrd_error, thrd_create(&thr, NULL, NULL));
}

void test_thrd_sleep_null(void) {
	assert_int_eq(-2, thrd_sleep(NULL, NULL));
}

static int return_value_func(void *arg) {
	return (int)(size_t)arg;
}

void test_thrd_create_join(void) {
	thrd_t thr;
	int    res;

	assert_int_eq(thrd_success, thrd_create(&thr, return_value_func, (void *)42));
	assert_int_eq(thrd_success, thrd_join(thr, &res));
	assert_int_eq(42, res);
}

static int simple_func(void *arg) {
	(void)arg;
	return 0;
}

void test_thrd_create_multiple(void) {
	int    i;
	thrd_t threads[NUM_THREADS];

	for (i = 0; i < NUM_THREADS; i++) {
		assert_int_eq(thrd_success, thrd_create(threads + i, simple_func, NULL));
	}
	for (i = 0; i < NUM_THREADS; i++) {
		assert_int_eq(thrd_success, thrd_join(threads[i], NULL));
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

void test_thrd_detach(void) {
	thrd_t          thr;
	struct timespec dur;

	assert_int_eq(thrd_success, thrd_create(&thr, detach_func, NULL));
	assert_int_eq(thrd_success, thrd_detach(thr));

	dur.tv_sec  = 0;
	dur.tv_nsec = 200000000;
	thrd_sleep(&dur, NULL);
}

void test_thrd_current(void) {
	thrd_t current = thrd_current();
	(void)current;
}

void test_thrd_equal(void) {
	thrd_t current = thrd_current();
	assert_true(thrd_equal(current, current));
}

void test_thrd_sleep(void) {
	struct timespec start, end, dur;

	dur.tv_sec  = 0;
	dur.tv_nsec = 100000000; /* 100ms */

	timespec_get(&start, TIME_UTC);
	assert_int_eq(0, thrd_sleep(&dur, NULL));
	timespec_get(&end, TIME_UTC);

	assert_int64_le(50000000L, (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec));
}

void test_thrd_yield(void) {
	thrd_yield();
}

int main(void) {
	cunit_init();

	CUNIT_SUITE_BEGIN("Input Validation", NULL, NULL)
	CUNIT_TEST("thrd_create with NULL thr", test_thrd_create_null_thr)
	CUNIT_TEST("thrd_create with NULL func", test_thrd_create_null_func)
	CUNIT_TEST("thrd_sleep with NULL", test_thrd_sleep_null)
	CUNIT_SUITE_END()

	CUNIT_SUITE_BEGIN("Functional", NULL, NULL)
	CUNIT_TEST("thrd_create and join", test_thrd_create_join)
	CUNIT_TEST("thrd_create multiple threads", test_thrd_create_multiple)
	CUNIT_TEST("thrd_detach", test_thrd_detach)
	CUNIT_TEST("thrd_current", test_thrd_current)
	CUNIT_TEST("thrd_equal", test_thrd_equal)
	CUNIT_TEST("thrd_sleep", test_thrd_sleep)
	CUNIT_TEST("thrd_yield", test_thrd_yield)
	CUNIT_SUITE_END()

	return cunit_run();
}
