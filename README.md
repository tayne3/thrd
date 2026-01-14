# thrd

![CMake](https://img.shields.io/badge/CMake-3.14%2B-brightgreen?logo=cmake&logoColor=white)
[![Release](https://img.shields.io/github/v/release/tayne3/thrd?include_prereleases&label=release&logo=github&logoColor=white)](https://github.com/tayne3/thrd/releases)
[![Tag](https://img.shields.io/github/v/tag/tayne3/thrd?color=%23ff8936&style=flat-square&logo=git&logoColor=white)](https://github.com/tayne3/thrd/tags)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/tayne3/thrd)

**English** | [中文](README_zh.md)

This library implements the C11 threads API (`<threads.h>`) on top of native platform threading primitives. It's designed to enable the use of C11 threading in C99 environments while maintaining compatibility with older compilers. This is **not** a drop-in replacement for C11's `<threads.h>`, but provides an equivalent API with the same function signatures and semantics.

The library has been tested on POSIX systems (macOS, Linux) and Windows. Contributions for additional platforms are welcome.

## Limitations

- **`thread_local` Keyword**: This library implements the function API but **cannot** provide the `thread_local` (or `_Thread_local`) keyword, as this is a compiler feature. Please use the `tss_*` functions for portable thread-local storage, or use compiler-specific equivalents (e.g., `__declspec(thread)` on MSVC).

---

## Supported Platforms

The following platforms are supported:

- **Windows Vista / Server 2008 or later**

  - Uses native Vista+ APIs: `CONDITION_VARIABLE`, `CRITICAL_SECTION`, `InitOnceExecuteOnce`, `FlsAlloc`
  - Supports MSVC, MinGW, Clang-cl
  - Does **not** support Windows XP or earlier

- **POSIX Systems**
  - macOS, Linux, BSD, and other POSIX-compliant systems
  - Uses pthreads (`pthread_create`, `pthread_mutex_*`, `pthread_cond_*`, etc.)
  - Supports GCC, Clang, and other POSIX-compatible compilers

---

## Implementation Details

### Windows (Win32)

The Win32 implementation uses modern Vista+ APIs for optimal performance:

- **Threads**: `_beginthreadex` / `_endthreadex` (CRT-safe)
- **Mutexes**: `CRITICAL_SECTION` (with emulation for non-recursive `mtx_trylock` and polling for `mtx_timedlock`)
- **Condition Variables**: `CONDITION_VARIABLE` (native Vista+ support)
- **Thread-Specific Storage**: `FlsAlloc` / `FlsSetValue` (with destructor support)
- **One-time Initialization**: `InitOnceExecuteOnce`

### POSIX

The POSIX implementation is a thin wrapper around pthreads:

- Direct mapping to `pthread_*` functions
- **macOS Special Handling**: `mtx_timedlock` uses polling emulation (macOS lacks `pthread_mutex_timedlock`)
- Proxy function for `thrd_create` to safely convert return types

---

## Usage

### CMake Integration

Add the library to your project:

```cmake
add_subdirectory(thrd)
target_link_libraries(my_app PRIVATE thrd::thrd)
```

### Basic Example

```c
#include <thrd/thrd.h>
#include <stdio.h>

int thread_func(void *arg) {
    int id = *(int *)arg;
    printf("Hello from thread %d\n", id);
    return 0;
}

int main(void) {
    thrd_t thread;
    int id = 42;

    if (thrd_create(&thread, thread_func, &id) != thrd_success) {
        return 1;
    }

    thrd_join(thread, NULL);
    return 0;
}
```

### Advanced Example

```c
#include <thrd/thrd.h>
#include <stdio.h>

mtx_t mutex;
cnd_t cond;
int ready = 0;

int worker(void *arg) {
    mtx_lock(&mutex);

    // Wait for signal
    while (!ready) {
        cnd_wait(&cond, &mutex);
    }

    printf("Worker received signal\n");
    mtx_unlock(&mutex);
    return 0;
}

int main(void) {
    thrd_t thread;

    mtx_init(&mutex, mtx_plain);
    cnd_init(&cond);

    thrd_create(&thread, worker, NULL);

    // Signal the worker
    mtx_lock(&mutex);
    ready = 1;
    cnd_signal(&cond);
    mtx_unlock(&mutex);

    thrd_join(thread, NULL);

    cnd_destroy(&cond);
    mtx_destroy(&mutex);
    return 0;
}
```

---

## API Reference

The library implements all C11 threads API functions with identical signatures:

```
+---------------------------+---------------------------+
| C11 <threads.h>           | thrd Library              |
+---------------------------+---------------------------+
| Thread Management                                     |
+---------------------------+---------------------------+
| thrd_create               | thrd_create               |
| thrd_exit                 | thrd_exit                 |
| thrd_join                 | thrd_join                 |
| thrd_detach               | thrd_detach               |
| thrd_current              | thrd_current              |
| thrd_equal                | thrd_equal                |
| thrd_sleep                | thrd_sleep                |
| thrd_yield                | thrd_yield                |
+---------------------------+---------------------------+
| Mutex Functions                                       |
+---------------------------+---------------------------+
| mtx_init                  | mtx_init                  |
| mtx_destroy               | mtx_destroy               |
| mtx_lock                  | mtx_lock                  |
| mtx_trylock               | mtx_trylock               |
| mtx_timedlock             | mtx_timedlock             |
| mtx_unlock                | mtx_unlock                |
+---------------------------+---------------------------+
| Condition Variables                                   |
+---------------------------+---------------------------+
| cnd_init                  | cnd_init                  |
| cnd_destroy               | cnd_destroy               |
| cnd_signal                | cnd_signal                |
| cnd_broadcast             | cnd_broadcast             |
| cnd_wait                  | cnd_wait                  |
| cnd_timedwait             | cnd_timedwait             |
+---------------------------+---------------------------+
| Thread-Specific Storage                               |
+---------------------------+---------------------------+
| tss_create                | tss_create                |
| tss_delete                | tss_delete                |
| tss_set                   | tss_set                   |
| tss_get                   | tss_get                   |
+---------------------------+---------------------------+
| One-time Initialization                               |
+---------------------------+---------------------------+
| call_once                 | call_once                 |
+---------------------------+---------------------------+
| Time Functions                                        |
+---------------------------+---------------------------+
| timespec_get              | timespec_get *            |
+---------------------------+---------------------------+
```

- `timespec_get` is only provided on platforms lacking native support (e.g., older MSVC).

---

## Differences With C11

While the API is designed to match C11 `<threads.h>` as closely as possible, there are some platform-specific behavioral differences:

### Windows-Specific Behavior

1. **Mutex Recursion**: Windows `CRITICAL_SECTION` is natively recursive. However, `mtx_trylock` explicitly checks and returns `thrd_busy` if a thread attempts to recursively lock an `mtx_plain` mutex, matching C11 semantics. `mtx_lock` remains recursive for `mtx_plain` to handle Undefined Behavior safely.

2. **Timed Mutexes**: Since `CRITICAL_SECTION` does not support timed waits, `mtx_timedlock` is implemented using a polling strategy (spin-wait with sleep) on Windows.

### POSIX-Specific Behavior

1. **macOS Timed Mutex**: On macOS, `mtx_timedlock` is emulated using polling with `pthread_mutex_trylock` because macOS lacks `pthread_mutex_timedlock`.

### General Notes

- Function signatures and return values match C11 exactly
- All type definitions (`thrd_t`, `mtx_t`, `cnd_t`, etc.) are compatible with C11
- Error codes (`thrd_success`, `thrd_error`, etc.) match C11 values

---

## Requirements

- **C99 Compiler**: The library requires C99 or later
- **Windows**: Windows Vista / Server 2008 or later
- **POSIX**: Any system with pthreads support
