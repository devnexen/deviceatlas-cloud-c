enable_testing()
add_executable(dacloud_tests dacloud_tests.c)
include_directories(../)
target_link_libraries(dacloud_tests dacloud ${CUNIT})
add_test(dacloud_tests dacloud_tests $ENV{HOME}/prod-file.conf)
