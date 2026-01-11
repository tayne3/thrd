cmake_minimum_required(VERSION 3.14)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/PreventInSourceBuilds.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/GitVersion.cmake)
git_version_info(VERSION THRD_VERSION DEFAULT_VERSION 0.1.0)
project(thrd VERSION ${THRD_VERSION})

include(${CMAKE_CURRENT_LIST_DIR}/cmake/ProjectIsTopLevel.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/OptionVariables.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/ProjectConfig.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/CPM.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/SetupThread.cmake)

if(THRD_BUILD_SHARED)
  add_library(thrd_shared SHARED)
  set(THRD_TARGET thrd_shared)
  message(STATUS "thrd v${THRD_VERSION} shared library")
else()
  add_library(thrd_static STATIC)
  set(THRD_TARGET thrd_static)
  message(STATUS "thrd v${THRD_VERSION} static library")
  target_compile_definitions(thrd_static PUBLIC THRD_STATIC)
endif()
add_library(thrd::thrd ALIAS ${THRD_TARGET})

target_link_libraries(${THRD_TARGET} PRIVATE thrd_compile_dependency)
target_include_directories(${THRD_TARGET} PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

if(THRD_BACKEND_WIN32)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config_win32.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/thrd/thrd_config.h @ONLY)
  target_sources(${THRD_TARGET} PRIVATE 
    src/win32/thread.c
    src/win32/mutex.c
    src/win32/cond.c
    src/win32/tss.c
    src/win32/call_once.c
    src/win32/time.c
  )
else()
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config_posix.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/thrd/thrd_config.h @ONLY)
  target_sources(${THRD_TARGET} PRIVATE 
    src/posix/thread.c
    src/posix/mutex.c
    src/posix/cond.c
    src/posix/tss.c
    src/posix/call_once.c
    src/posix/time.c
  )
endif()

include(GenerateExportHeader)
generate_export_header(${THRD_TARGET}
	BASE_NAME thrd
	EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/thrd/thrd_export.h
)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/include/thrd/thrd_config.h
	${CMAKE_CURRENT_BINARY_DIR}/include/thrd/thrd_export.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thrd
)

if(THRD_BUILD_TEST)
  CPMAddPackage(
    NAME cunit
    URL "https://codeload.github.com/tayne3/cunit/tar.gz/refs/tags/v0.2.4"
  )
  enable_testing()
  add_subdirectory(test)
endif()
