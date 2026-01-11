# thrd

![CMake](https://img.shields.io/badge/CMake-3.14%2B-brightgreen?logo=cmake&logoColor=white)
[![Release](https://img.shields.io/github/v/release/tayne3/thrd?include_prereleases&label=release&logo=github&logoColor=white)](https://github.com/tayne3/thrd/releases)
[![Tag](https://img.shields.io/github/v/tag/tayne3/thrd?color=%23ff8936&style=flat-square&logo=git&logoColor=white)](https://github.com/tayne3/thrd/tags)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/tayne3/thrd)

[English](README.md) | **中文**

本库基于原生平台线程原语实现了 C11 线程 API（`<threads.h>`）。它旨在让 C99 环境也能使用 C11 线程功能，同时保持对旧版本编译器的兼容性。这**不是** C11 `<threads.h>` 的完全替代品，但提供了一个具有相同函数签名和语义的等效 API。

该库已在 POSIX 系统（macOS、Linux）和 Windows 上进行了测试。欢迎为其他平台贡献代码。

---

## 支持的平台

目前支持以下平台：

- **Windows Vista / Server 2008 或更高版本**

  - 使用原生 Vista+ API：`CONDITION_VARIABLE`、`CRITICAL_SECTION`、`InitOnceExecuteOnce`、`FlsAlloc`
  - 支持 MSVC、MinGW、Clang-cl
  - **不支持** Windows XP 或更早版本

- **POSIX 系统**
  - macOS、Linux、BSD 及其他符合 POSIX 标准的系统
  - 使用 pthreads（`pthread_create`、`pthread_mutex_*`、`pthread_cond_*` 等）
  - 支持 GCC、Clang 和其他兼容 POSIX 的编译器

---

## 实现细节

### Windows (Win32)

Win32 实现采用现代 Vista+ API 以获得最佳性能：

- **线程 (Threads)**: `_beginthreadex` / `_endthreadex`（CRT 安全）
- **互斥锁 (Mutexes)**: `CRITICAL_SECTION`（始终是递归的）
- **条件变量 (Condition Variables)**: `CONDITION_VARIABLE`（原生 Vista+ 支持）
- **线程特定存储 (Thread-Specific Storage)**: `FlsAlloc` / `FlsSetValue`（支持析构函数）
- **单次初始化 (One-time Initialization)**: `InitOnceExecuteOnce`

### POSIX

POSIX 实现是对 pthreads 的薄封装：

- 直接映射到 `pthread_*` 函数
- **macOS 特殊处理**: 由于 macOS 缺少 `pthread_mutex_timedlock`，`mtx_timedlock` 使用轮询模拟实现
- `thrd_create` 的代理函数，用于安全地转换返回类型

---

## 使用方法

### CMake 集成

将本库添加到您的项目中：

```cmake
add_subdirectory(thrd)
target_link_libraries(my_app PRIVATE thrd::thrd)
```

### 基础示例

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

### 进阶示例

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

## API 参考

本库实现了所有 C11 线程 API 函数，具有完全相同的签名：

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

- `timespec_get` 仅在缺少原生支持的平台（例如旧版 MSVC）上提供。

---

## 与 C11 的区别

虽然 API 旨在尽可能贴合 C11 `<threads.h>`，但在不同平台上仍存在一些行为差异：

### Windows 特定行为

1. **互斥锁递归**: Windows `CRITICAL_SECTION` 始终是递归的。在 Windows 上，`mtx_plain` 和 `mtx_recursive` 的行为完全一致（同一个线程可以多次锁定同一个互斥锁而不会死锁）。

2. **互斥锁类型**: `mtx_timed` 标志会被接受，但在 Windows 上没有特殊效果（所有互斥锁都支持定时操作）。

### POSIX 特定行为

1. **macOS 定时互斥锁**: 在 macOS 上，由于缺少 `pthread_mutex_timedlock`，`mtx_timedlock` 使用 `pthread_mutex_trylock` 进行轮询模拟。

### 通用注意事项

- 函数签名和返回值与 C11 完全一致
- 所有类型定义（`thrd_t`、`mtx_t`、`cnd_t` 等）均与 C11 兼容
- 错误代码（`thrd_success`、`thrd_error` 等）与 C11 的值匹配

---

## 环境要求

- **C99 编译器**: 本库要求 C99 或更高版本
- **Windows**: Windows Vista / Server 2008 或更高版本
- **POSIX**: 任何支持 pthreads 的系统
