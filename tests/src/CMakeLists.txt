cmake_minimum_required(VERSION 3.10)

project(unit_tests)

add_executable(
        ${PROJECT_NAME}
        unit_tests.cxx
)

target_link_libraries(
        ${PROJECT_NAME}
        gtest_main
        queue_lib
        gmock
        benchmark::benchmark
)

add_executable(
        benchmarks
        benchmarks.cxx
)

target_link_libraries(
        benchmarks
        queue_lib
        gtest_main
        gmock
        benchmark::benchmark
)

add_test(
        NAME
            multi_queue_unit_tests
        COMMAND
            $<TARGET_FILE:unit_tests>
)
