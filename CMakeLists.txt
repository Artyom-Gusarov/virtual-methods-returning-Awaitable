cmake_minimum_required(VERSION 3.14)

project(VirtualAwaiter)

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
    benchmark_dynamic_awaiter_lambdas
    benchmark/shared_data_case.cpp
)
target_compile_definitions(benchmark_dynamic_awaiter_lambdas PRIVATE TEST_DYNAMIC_AWAITER_LAMBDAS)
target_link_libraries(benchmark_dynamic_awaiter_lambdas benchmark::benchmark)

add_executable(
    benchmark_dynamic_awaiter_virtual_interface
    benchmark/shared_data_case.cpp
)
target_compile_definitions(benchmark_dynamic_awaiter_virtual_interface PRIVATE TEST_DYNAMIC_AWAITER_VIRTUAL_INTERFACE)
target_link_libraries(benchmark_dynamic_awaiter_virtual_interface benchmark::benchmark)

add_executable(
    benchmark_dynamic_awaiter_virtual_base
    benchmark/shared_data_case.cpp
)
target_compile_definitions(benchmark_dynamic_awaiter_virtual_base PRIVATE TEST_DYNAMIC_AWAITER_VIRTUAL_BASE)
target_link_libraries(benchmark_dynamic_awaiter_virtual_base benchmark::benchmark)



add_custom_target(run-all-benchmarks
    DEPENDS benchmark_dynamic_awaiter_lambdas benchmark_dynamic_awaiter_virtual_interface benchmark_dynamic_awaiter_virtual_base
    COMMAND benchmark_dynamic_awaiter_lambdas --benchmark_out=${CMAKE_SOURCE_DIR}/benchmark/results/dynamic_awaiter_lambdas.txt --benchmark_out_format=console
    COMMAND benchmark_dynamic_awaiter_virtual_interface --benchmark_out=${CMAKE_SOURCE_DIR}/benchmark/results/dynamic_awaiter_virtual_interface.txt --benchmark_out_format=console
    COMMAND benchmark_dynamic_awaiter_virtual_base --benchmark_out=${CMAKE_SOURCE_DIR}/benchmark/results/dynamic_awaiter_virtual_base.txt --benchmark_out_format=console
)

add_custom_target(init_all_builds
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/debug_build
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/release_build
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/tsan_build
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_SOURCE_DIR}/debug_build -DCMAKE_BUILD_TYPE=Debug
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_SOURCE_DIR}/release_build -DCMAKE_BUILD_TYPE=Release
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_SOURCE_DIR}/tsan_build -DCMAKE_BUILD_TYPE=TSAN
)
