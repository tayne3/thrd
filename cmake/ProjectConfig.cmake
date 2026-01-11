include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCSourceCompiles)
include(CheckStructHasMember)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

add_library(thrd_compile_dependency INTERFACE)
target_compile_features(thrd_compile_dependency INTERFACE c_std_99)

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

check_struct_has_member("struct timespec" "tv_sec" "time.h" HAVE_TIMESPEC)
check_c_source_compiles("
#include <time.h>
int main(void) {
  struct timespec ts;
  return timespec_get(&ts, TIME_UTC) != TIME_UTC;
}
" HAVE_TIMESPEC_GET)
