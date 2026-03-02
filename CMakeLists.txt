cmake_minimum_required(VERSION 3.14)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(
    FATAL_ERROR
    "In-source builds are not supported. "
    "You may need to delete 'CMakeCache.txt' and 'CMakeFiles/' before rebuilding this project."
  )
endif()

project(thrd VERSION 0.1.0)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCSourceCompiles)
include(CheckStructHasMember)
include(CMakePushCheckState)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  option(THRD_USE_NATIVE "use native <threads.h>" OFF)
  option(THRD_BUILD_TEST "build test program" OFF)

  get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(NOT isMultiConfig
     AND NOT CMAKE_BUILD_TYPE
     AND NOT CMAKE_CONFIGURATION_TYPES
  )
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
  endif()
    
  if(NOT DEFINED CMAKE_C_VISIBILITY_PRESET)
    set(CMAKE_C_VISIBILITY_PRESET hidden CACHE STRING "Preset for the export of private symbols")
    set_property(CACHE CMAKE_C_VISIBILITY_PRESET PROPERTY STRINGS hidden default)
  endif()

  if(NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden CACHE STRING "Preset for the export of private symbols")
    set_property(CACHE CMAKE_CXX_VISIBILITY_PRESET PROPERTY STRINGS hidden default)
  endif()

  if(NOT DEFINED CMAKE_VISIBILITY_INLINES_HIDDEN)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON CACHE BOOL "Whether to add a compile flag to hide symbols of inline functions")
  endif()
  
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  
  include(GNUInstallDirs)
  set(CMAKE_SKIP_BUILD_RPATH OFF)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
  if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_INSTALL_RPATH "@loader_path;@loader_path/${CMAKE_INSTALL_LIBDIR}")
  else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN;$ORIGIN/${CMAKE_INSTALL_LIBDIR}")
  endif()
endif()

add_library(thrd_compile_dependency INTERFACE)
target_compile_features(thrd_compile_dependency INTERFACE c_std_99)
add_library(thrd_public_dependency INTERFACE)
target_compile_features(thrd_public_dependency INTERFACE c_std_99)

# set source charset to utf-8 for MSVC
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(thrd_compile_dependency INTERFACE 
		"$<$<COMPILE_LANGUAGE:C>:/utf-8>"
	)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(thrd_compile_dependency INTERFACE 
		"$<$<COMPILE_LANGUAGE:CXX>:/utf-8>"
	)
endif()

# compiler warnings, skipped for MSVC & ClangCL (MSVC frontend)
if(NOT (CMAKE_C_COMPILER_ID STREQUAL "MSVC" OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" AND "${CMAKE_C_COMPILER_FRONTEND_VARIANT}" STREQUAL "MSVC")))
  target_compile_options(thrd_compile_dependency INTERFACE
    "$<$<COMPILE_LANGUAGE:C>:-Wall>"
    "$<$<COMPILE_LANGUAGE:C>:-Wextra>"
  )
  check_c_compiler_flag("-Werror=return-type" HAVE_C_WERROR_RETURN_TYPE)
  if(HAVE_C_WERROR_RETURN_TYPE)
    target_compile_options(thrd_compile_dependency INTERFACE
      "$<$<COMPILE_LANGUAGE:C>:-Werror=return-type>"
    )
  endif()
endif()
if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND "${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "MSVC")))
  target_compile_options(thrd_compile_dependency INTERFACE
    "$<$<COMPILE_LANGUAGE:CXX>:-Wall>"
    "$<$<COMPILE_LANGUAGE:CXX>:-Wextra>"
  )
  check_cxx_compiler_flag("-Werror=return-type" HAVE_CXX_WERROR_RETURN_TYPE)
  if(HAVE_CXX_WERROR_RETURN_TYPE)
    target_compile_options(thrd_compile_dependency INTERFACE
      "$<$<COMPILE_LANGUAGE:CXX>:-Werror=return-type>"
    )
  endif()
endif()

# Handle -Wno-missing-field-initializers for older GCC
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_LESS 6.0)
  target_compile_options(thrd_compile_dependency INTERFACE 
		"$<$<COMPILE_LANGUAGE:C>:-Wno-missing-field-initializers>"
	)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
  target_compile_options(thrd_compile_dependency INTERFACE 
		"$<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers>"
	)
endif()

check_struct_has_member("struct timespec" "tv_sec" "time.h" THRD_HAVE_TIMESPEC)
check_c_source_compiles("
#include <time.h>
int main(void) {
  struct timespec ts;
  return timespec_get(&ts, TIME_UTC) != TIME_UTC;
}
" THRD_HAVE_TIMESPEC_GET)

if(THRD_USE_NATIVE)
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_C_FLAGS}")
  set(CMAKE_C_STANDARD 11)
  check_c_source_compiles("
  #include <threads.h>
  int func(void *arg) { (void)arg; return 0; }
  int main(void) {
      thrd_t t;
      mtx_t m;
      cnd_t c;
      if (thrd_create(&t, func, 0) != thrd_success) return 1;
      thrd_join(t, 0);
      mtx_init(&m, mtx_plain);
      mtx_destroy(&m);
      cnd_init(&c);
      cnd_destroy(&c);
      return 0;
  }
  " THRD_SUPPORT_NATIVE)
  cmake_pop_check_state()
endif()

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
  if(BUILD_SHARED_LIBS)
    set(THRD_LIB_TYPE SHARED)
  else()
    set(THRD_LIB_TYPE STATIC)
  endif()
  message(STATUS "thrd v${PROJECT_VERSION} ${THRD_LIB_TYPE} library")

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
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/src/win32/config_win32.h.in
      ${CMAKE_CURRENT_BINARY_DIR}/include/thrd/thrd_config.h
      @ONLY
    )
    target_sources(thrd PRIVATE 
      src/win32/thread.c
      src/win32/mutex.c
      src/win32/cond.c
      src/win32/tss.c
      src/win32/call_once.c
      src/win32/time.c
    )
  else()
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/src/posix/config_posix.h.in
      ${CMAKE_CURRENT_BINARY_DIR}/include/thrd/thrd_config.h
      @ONLY
    )
    target_sources(thrd PRIVATE 
      src/posix/thread.c
      src/posix/mutex.c
      src/posix/cond.c
      src/posix/tss.c
      src/posix/call_once.c
      src/posix/time.c
    )
  endif()

  install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/include/thrd/thrd_config.h
    DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}/thrd
  )
endif()

message("THRD_BUILD_TEST: ${THRD_BUILD_TEST}")
if(THRD_BUILD_TEST)
  enable_testing()
  add_subdirectory(test)
endif()
