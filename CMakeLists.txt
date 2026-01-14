cmake_minimum_required(VERSION 3.14)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/PreventInSourceBuilds.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/GitVersion.cmake)
git_version_info(VERSION THRD_VERSION DEFAULT_VERSION 0.1.0)
project(thrd VERSION ${THRD_VERSION})

include(${CMAKE_CURRENT_LIST_DIR}/cmake/ProjectConfig.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/CPM.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/SetupThread.cmake)

if(THRD_USE_NATIVE AND THRD_SUPPORT_NATIVE)
  message(STATUS "thrd: Using system <threads.h>")
  set(CMAKE_C_STANDARD 11)
  add_library(thrd_native INTERFACE)
  add_library(thrd::thrd ALIAS thrd_native)
  target_include_directories(thrd_native INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  )
  target_link_libraries(thrd PUBLIC thrd_public_dependency)
else()
  set(THRD_LIB_TYPE STATIC)
  if(BUILD_SHARED_LIBS)
    set(THRD_LIB_TYPE SHARED)
  endif()
  message(STATUS "thrd v${THRD_VERSION} ${THRD_LIB_TYPE} library")

  add_library(thrd ${THRD_LIB_TYPE})
  add_library(thrd::thrd ALIAS thrd)
  target_link_libraries(thrd PRIVATE thrd_compile_dependency)
  target_link_libraries(thrd PUBLIC thrd_public_dependency)
  target_include_directories(thrd PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
  )
  if(BUILD_SHARED_LIBS)
    target_compile_definitions(thrd PRIVATE THRD_LIB_EXPORT INTERFACE THRD_SHARED)
  endif()

  if(THRD_BACKEND_WIN32)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config_win32.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/thrd/thrd_config.h @ONLY)
    target_sources(thrd PRIVATE 
      src/win32/thread.c
      src/win32/mutex.c
      src/win32/cond.c
      src/win32/tss.c
      src/win32/call_once.c
      src/win32/time.c
    )
  else()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config_posix.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/thrd/thrd_config.h @ONLY)
    target_sources(thrd PRIVATE 
      src/posix/thread.c
      src/posix/mutex.c
      src/posix/cond.c
      src/posix/tss.c
      src/posix/call_once.c
      src/posix/time.c
    )
  endif()

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/include/thrd/thrd_config.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thrd
  )
endif()

if(THRD_BUILD_TEST)
  CPMAddPackage(
    NAME cunit
    URL "https://codeload.github.com/tayne3/cunit/tar.gz/refs/tags/v0.2.5"
    URL_HASH "SHA256=73429bf8bc733ce5431b012689e56f9d255bab3d6d1ac745311f81514b322186"
    OPTIONS "BUILD_SHARED_LIBS OFF"
  )
  enable_testing()
  add_subdirectory(test)
endif()
