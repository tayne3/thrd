add_executable(thread_test thread_test.c)
add_test(NAME thread_test COMMAND thread_test)
target_link_libraries(thread_test PRIVATE thrd::thrd cunit::cunit)

add_executable(mutex_test mutex_test.c)
add_test(NAME mutex_test COMMAND mutex_test)
target_link_libraries(mutex_test PRIVATE thrd::thrd cunit::cunit)

add_executable(cond_test cond_test.c)
add_test(NAME cond_test COMMAND cond_test)
target_link_libraries(cond_test PRIVATE thrd::thrd cunit::cunit)

add_executable(tss_test tss_test.c)
add_test(NAME tss_test COMMAND tss_test)
target_link_libraries(tss_test PRIVATE thrd::thrd cunit::cunit)

add_executable(call_once_test call_once_test.c)
add_test(NAME call_once_test COMMAND call_once_test)
target_link_libraries(call_once_test PRIVATE thrd::thrd cunit::cunit)
