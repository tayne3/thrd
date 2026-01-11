/*
 * MIT License
 *
 * Copyright (c) 2026 tayne3
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be included in
 *    all copies or substantial portions of the Software.
 *
 * 2. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *    SOFTWARE.
 */
#ifndef THRD_THRD_H
#define THRD_THRD_H

#include <stddef.h>
#include <thrd/thrd_config.h>
#include <thrd/thrd_export.h>
#include <time.h>

#ifndef TIME_UTC
#define TIME_UTC 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*thrd_start_t)(void *);
typedef void (*tss_dtor_t)(void *);

enum { mtx_plain = 0, mtx_recursive = 1, mtx_timed = 2 };
enum { thrd_success = 0, thrd_timedout, thrd_busy, thrd_error, thrd_nomem };

THRD_EXPORT int    thrd_create(thrd_t *thr, thrd_start_t func, void *arg);
THRD_EXPORT void   thrd_exit(int res);
THRD_EXPORT int    thrd_join(thrd_t thr, int *res);
THRD_EXPORT int    thrd_detach(thrd_t thr);
THRD_EXPORT thrd_t thrd_current(void);
THRD_EXPORT int    thrd_equal(thrd_t a, thrd_t b);
THRD_EXPORT int    thrd_sleep(const struct timespec *ts_in, struct timespec *rem_out);
THRD_EXPORT void   thrd_yield(void);

THRD_EXPORT int  mtx_init(mtx_t *mtx, int type);
THRD_EXPORT void mtx_destroy(mtx_t *mtx);
THRD_EXPORT int  mtx_lock(mtx_t *mtx);
THRD_EXPORT int  mtx_trylock(mtx_t *mtx);
THRD_EXPORT int  mtx_timedlock(mtx_t *mtx, const struct timespec *ts);
THRD_EXPORT int  mtx_unlock(mtx_t *mtx);

THRD_EXPORT int  cnd_init(cnd_t *cond);
THRD_EXPORT void cnd_destroy(cnd_t *cond);
THRD_EXPORT int  cnd_signal(cnd_t *cond);
THRD_EXPORT int  cnd_broadcast(cnd_t *cond);
THRD_EXPORT int  cnd_wait(cnd_t *cond, mtx_t *mtx);
THRD_EXPORT int  cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts);

THRD_EXPORT int   tss_create(tss_t *key, tss_dtor_t dtor);
THRD_EXPORT void  tss_delete(tss_t key);
THRD_EXPORT int   tss_set(tss_t key, void *val);
THRD_EXPORT void *tss_get(tss_t key);

THRD_EXPORT void call_once(once_flag *flag, void (*func)(void));

#ifndef HAVE_TIMESPEC_GET
THRD_EXPORT int timespec_get(struct timespec *ts, int base);
#endif

#ifdef __cplusplus
}
#endif

#endif /* THRD_THRD_H */
