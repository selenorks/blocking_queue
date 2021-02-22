cmake_minimum_required(VERSION 3.10)

project(unit_tests)

include_directories(../../benchmark/googletest/googlemock/include)
add_executable(
        ${PROJECT_NAME}
        unit_tests.cxx
)

target_link_libraries(
        ${PROJECT_NAME}
        queue_lib
        benchmark::benchmark
        gtest_main
)

add_executable(
        benchmarks
        benchmarks.cxx
)

target_link_libraries(
        benchmarks
        queue_lib
        gtest_main
        benchmark::benchmark
)

add_test(
        NAME
            queue_unit_tests
        COMMAND
            $<TARGET_FILE:unit_tests>
)
