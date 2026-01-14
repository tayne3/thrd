#include <thrd/thrd.h>

#include "internal.h"

static BOOL CALLBACK _thrd_once_proxy(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context) {
	void (*func)(void) = (void (*)(void))Parameter;
	(void)InitOnce;
	(void)Context;
	func();
	return TRUE;
}

void call_once(once_flag *flag, void (*func)(void)) {
	if (!flag || !func) {
		return;
	}
	InitOnceExecuteOnce((PINIT_ONCE)&flag->Ptr, _thrd_once_proxy, (void *)func, NULL);
}
