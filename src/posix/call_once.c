#include <thrd/thrd.h>

void call_once(once_flag *flag, void (*func)(void)) {
	if (!flag || !func)
		return;
	pthread_once(flag, func);
}
