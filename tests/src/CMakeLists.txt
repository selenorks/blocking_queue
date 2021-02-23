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

add_test(
        NAME
            unit_tests
        COMMAND
            $<TARGET_FILE:unit_tests>
)

add_executable(
        compilation_test
        compilation_test.cxx
)

target_link_libraries(
        compilation_test
        queue_lib
)

target_compile_options(compilation_test PRIVATE -fno-rtti -fno-exceptions)

add_test(
        NAME
            compilation_test
        COMMAND
            $<TARGET_FILE:compilation_test>
)
