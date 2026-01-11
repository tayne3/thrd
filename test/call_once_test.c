#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "cunit.h"
#include "thrd/thrd.h"

#define NUM_THREADS 8

static once_flag shared_once = ONCE_FLAG_INIT;
static int       shared_flag;

void test_call_once_null_both(void) {
	call_once(NULL, NULL);
}

static once_flag single_thread_once = ONCE_FLAG_INIT;
static int       single_thread_counter;

static void single_thread_func(void) {
	cunit_println("single_thread_func() was called");
	++single_thread_counter;
}

void test_call_once_single_thread(void) {
	single_thread_counter = 0;

	call_once(&single_thread_once, single_thread_func);
	call_once(&single_thread_once, single_thread_func);
	call_once(&single_thread_once, single_thread_func);

	assert_int_eq(1, single_thread_counter);
}

static void call_once_func(void) {
	cunit_println("call_once_func() was called");
	++shared_flag;
}

static int call_once_thread_func(void *arg) {
	(void)arg;
	cunit_println("call_once_thread_func() was called");
	call_once(&shared_once, call_once_func);
	return 0;
}

void test_call_once_multiple_threads(void) {
	int    i;
	thrd_t threads[NUM_THREADS];

	shared_flag = 0;

	for (i = 0; i < NUM_THREADS; i++) {
		assert_int_eq(thrd_success, thrd_create(threads + i, call_once_thread_func, NULL));
	}

	for (i = 0; i < NUM_THREADS; i++) {
		assert_int_eq(thrd_success, thrd_join(threads[i], NULL));
	}

	cunit_println("content of flag: %d", shared_flag);
	assert_int_eq(shared_flag, 1);
}

int main(void) {
	cunit_init();

	CUNIT_SUITE_BEGIN("Input Validation", NULL, NULL)
	CUNIT_TEST("call_once with both NULL", test_call_once_null_both)
	CUNIT_SUITE_END()

	CUNIT_SUITE_BEGIN("Functional", NULL, NULL)
	CUNIT_TEST("call_once single thread", test_call_once_single_thread)
	CUNIT_SUITE_END()

	CUNIT_SUITE_BEGIN("Concurrency", NULL, NULL)
	CUNIT_TEST("call_once multiple threads", test_call_once_multiple_threads)
	CUNIT_SUITE_END()

	const int ret = cunit_run();
#ifdef _MSC_VER
	if (_CrtDumpMemoryLeaks()) {
		abort();
	}
#endif
	return ret;
}
