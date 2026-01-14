add_library(Catch2 INTERFACE)
target_include_directories(Catch2 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(Catch2Main INTERFACE)
target_include_directories(Catch2Main INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(Catch2Main INTERFACE CATCH_CONFIG_MAIN)

add_executable(thread_test thread_test.cpp)
target_link_libraries(thread_test PRIVATE thrd::thrd Catch2Main)
add_test(NAME thread_test COMMAND thread_test)

add_executable(mutex_test mutex_test.cpp)
target_link_libraries(mutex_test PRIVATE thrd::thrd Catch2Main)
add_test(NAME mutex_test COMMAND mutex_test)

add_executable(cond_test cond_test.cpp)
target_link_libraries(cond_test PRIVATE thrd::thrd Catch2Main)
add_test(NAME cond_test COMMAND cond_test)

add_executable(tss_test tss_test.cpp)
target_link_libraries(tss_test PRIVATE thrd::thrd Catch2Main)
add_test(NAME tss_test COMMAND tss_test)

add_executable(call_once_test call_once_test.cpp)
target_link_libraries(call_once_test PRIVATE thrd::thrd Catch2Main)
add_test(NAME call_once_test COMMAND call_once_test)
