#include "cunit.h"
#include "thrd/thrd.h"

#define NUM_THREADS 4

static tss_t shared_tss;
static int   dtor_called;
static mtx_t dtor_mtx;

void test_tss_create_null(void) {
	assert_int_eq(thrd_error, tss_create(NULL, NULL));
}

void test_tss_set_get(void) {
	tss_t tss;
	void *val;

	assert_int_eq(thrd_success, tss_create(&tss, NULL));

	assert_int_eq(thrd_success, tss_set(tss, (void *)42));
	val = tss_get(tss);
	assert_int_eq(42, (int)(size_t)val);

	assert_int_eq(thrd_success, tss_set(tss, (void *)100));
	val = tss_get(tss);
	assert_int_eq(100, (int)(size_t)val);

	tss_delete(tss);
}

static void dtor_func(void *arg) {
	cunit_println("dtor: content of tss: %d", (int)(size_t)arg);
	assert_int_eq((int)(size_t)arg, 42);
	mtx_lock(&dtor_mtx);
	dtor_called++;
	mtx_unlock(&dtor_mtx);
}

static int tss_thread_func(void *arg) {
	void *tss_content;
	(void)arg;

	tss_content = tss_get(shared_tss);
	cunit_println("thread func: initial content of tss: %d", (int)(size_t)tss_content);
	assert_int_eq(thrd_success, tss_set(shared_tss, (void *)42));
	tss_content = tss_get(shared_tss);
	cunit_println("thread func: content of tss now: %d", (int)(size_t)tss_content);
	assert_int_eq((int)(size_t)tss_content, 42);

	return 0;
}

void test_tss_dtor_called(void) {
	thrd_t thread;

	assert_int_eq(thrd_success, mtx_init(&dtor_mtx, mtx_plain));
	dtor_called = 0;
	assert_int_eq(thrd_success, tss_create(&shared_tss, dtor_func));
	assert_int_eq(thrd_success, thrd_create(&thread, tss_thread_func, NULL));
	assert_int_eq(thrd_success, thrd_join(thread, NULL));

	assert_int_eq(1, dtor_called);

	tss_delete(shared_tss);
	mtx_destroy(&dtor_mtx);
}

static int isolation_thread_func(void *arg) {
	int   thread_num = (int)(size_t)arg;
	void *val;

	assert_int_eq(thrd_success, tss_set(shared_tss, (void *)(size_t)(thread_num * 10)));

	struct timespec dur = {0, 10000000};
	thrd_sleep(&dur, NULL);

	val = tss_get(shared_tss);
	assert_int_eq(thread_num * 10, (int)(size_t)val);

	return 0;
}

void test_tss_isolation(void) {
	int    i;
	thrd_t threads[NUM_THREADS];

	assert_int_eq(thrd_success, tss_create(&shared_tss, NULL));

	for (i = 0; i < NUM_THREADS; i++) {
		assert_int_eq(thrd_success, thrd_create(threads + i, isolation_thread_func, (void *)(size_t)(i + 1)));
	}
	for (i = 0; i < NUM_THREADS; i++) {
		assert_int_eq(thrd_success, thrd_join(threads[i], NULL));
	}

	tss_delete(shared_tss);
}

void test_tss_multiple_keys(void) {
	tss_t tss1, tss2;
	void *val1, *val2;

	assert_int_eq(thrd_success, tss_create(&tss1, NULL));
	assert_int_eq(thrd_success, tss_create(&tss2, NULL));

	assert_int_eq(thrd_success, tss_set(tss1, (void *)10));
	assert_int_eq(thrd_success, tss_set(tss2, (void *)20));

	val1 = tss_get(tss1);
	val2 = tss_get(tss2);

	assert_int_eq(10, (int)(size_t)val1);
	assert_int_eq(20, (int)(size_t)val2);

	tss_delete(tss2);
	tss_delete(tss1);
}

int main(void) {
	cunit_init();

	CUNIT_SUITE_BEGIN("Input Validation", NULL, NULL)
	CUNIT_TEST("tss_create with NULL", test_tss_create_null)
	CUNIT_SUITE_END()

	CUNIT_SUITE_BEGIN("Functional", NULL, NULL)
	CUNIT_TEST("tss_set and tss_get", test_tss_set_get)
	CUNIT_TEST("tss destructor called", test_tss_dtor_called)
	CUNIT_TEST("tss isolation between threads", test_tss_isolation)
	CUNIT_TEST("tss multiple keys", test_tss_multiple_keys)
	CUNIT_SUITE_END()

	return cunit_run();
}
