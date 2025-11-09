cmake_minimum_required(VERSION 3.14)

project(DynamicAwaiter)

# No Werror for google test and benchmark 
set(CMAKE_CXX_FLAGS_TSAN "-O2 -Wall -Wextra -Wpedantic -fsanitize=thread -g")
set(CMAKE_CXX_FLAGS_DEBUG "-O2 -Wall -Wextra -Wpedantic -fsanitize=address,undefined,leak -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -Wextra -Wpedantic")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.9.4.zip
)
set(BENCHMARK_ENABLE_WERROR OFF)
FetchContent_MakeAvailable(googlebenchmark)



set(CMAKE_CXX_FLAGS_TSAN "-O2 -Wall -Wextra -Wpedantic -Werror -fsanitize=thread -g")
set(CMAKE_CXX_FLAGS_DEBUG "-O2 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined,leak -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -Wextra -Wpedantic -Werror")



add_executable(
    benchmark_dynamic_awaiter
    benchmark/shared_data_case.cpp
)
target_compile_definitions(benchmark_dynamic_awaiter PRIVATE TEST_DYNAMIC_AWAITER)
target_link_libraries(benchmark_dynamic_awaiter benchmark::benchmark)

add_executable(
    benchmark_new_future
    benchmark/shared_data_case.cpp
)
target_compile_definitions(benchmark_new_future PRIVATE TEST_NEW_FUTURE)
target_link_libraries(benchmark_new_future benchmark::benchmark)



add_executable(
    test_dynamic_awaiter
    tests/dynamic_awaiter.cpp
)
target_link_libraries(test_dynamic_awaiter gtest_main)



add_custom_target(run-all-benchmarks
    DEPENDS benchmark_dynamic_awaiter benchmark_new_future
    COMMAND benchmark_dynamic_awaiter --benchmark_out=${CMAKE_SOURCE_DIR}/benchmark/results/benchmark_dynamic_awaiter.txt --benchmark_out_format=console
    COMMAND benchmark_new_future --benchmark_out=${CMAKE_SOURCE_DIR}/benchmark/results/benchmark_new_future.txt --benchmark_out_format=console
)

add_custom_target(init_all_builds
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/debug_build
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/release_build
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/tsan_build
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_SOURCE_DIR}/debug_build -DCMAKE_BUILD_TYPE=Debug
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_SOURCE_DIR}/release_build -DCMAKE_BUILD_TYPE=Release
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_SOURCE_DIR}/tsan_build -DCMAKE_BUILD_TYPE=TSAN
)
