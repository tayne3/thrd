# Detect Backend
include_guard(GLOBAL)

include(CheckLibraryExists)

# ------------------------------------------------------------------------------
# Try to find system-provided pthreads
# ------------------------------------------------------------------------------

# Prefer pthreads if available (e.g., Linux, macOS, MinGW)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads QUIET)
if(CMAKE_USE_PTHREADS_INIT)
  target_link_libraries(thrd_public_dependency INTERFACE ${CMAKE_THREAD_LIBS_INIT})
  return()
endif()

# ------------------------------------------------------------------------------
# Windows/MSVC Fallback
# ------------------------------------------------------------------------------
# If we reach here, the system does not provide pthreads (common on MSVC).
# We need to either find an external pthreads library or build one.

# Check for existing pthreads libraries in the library path
check_library_exists(pthreads pthread_create "" CMAKE_HAVE_PTHREADS_CREATE)
if(CMAKE_HAVE_PTHREADS_CREATE)
  target_link_libraries(thrd_public_dependency INTERFACE "-lpthreads")
  return()
endif()
check_library_exists(pthread pthread_create "" CMAKE_HAVE_PTHREAD_CREATE)
if(CMAKE_HAVE_PTHREAD_CREATE)
  target_link_libraries(thrd_public_dependency INTERFACE "-lpthread")
  return()
endif()

if(WIN32)
  set(THRD_BACKEND_WIN32 ON)
else()
  message(FATAL_ERROR "No threading backend available on this platform")
endif()
