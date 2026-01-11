#ifndef _WIN32

#include <thrd/thrd.h>

int tss_create(tss_t *key, tss_dtor_t dtor) {
	if (!key)
		return thrd_error;
	return pthread_key_create(key, dtor) == 0 ? thrd_success : thrd_error;
}

void tss_delete(tss_t key) {
	pthread_key_delete(key);
}

int tss_set(tss_t key, void *val) {
	return pthread_setspecific(key, val) == 0 ? thrd_success : thrd_error;
}

void *tss_get(tss_t key) {
	return pthread_getspecific(key);
}

#endif /* !_WIN32 */
