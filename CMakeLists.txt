cmake_minimum_required(VERSION 3.18)
project(options_pricing_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimised by default; this matters once we start benchmarking latency.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
# Keep -DNDEBUG: without it, Release builds still run asserts and tools like
# Google Benchmark report being built in debug mode.
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# --- Core library (pure C++, no Python dependency) ---
add_library(ope_core STATIC
    src/black_scholes.cpp
    src/binomial.cpp
    src/monte_carlo.cpp
    src/implied_vol.cpp
    src/jump_diffusion.cpp)
target_include_directories(ope_core PUBLIC include)
set_target_properties(ope_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- C++ test executables ---
add_executable(test_black_scholes tests/test_black_scholes.cpp)
target_link_libraries(test_black_scholes PRIVATE ope_core)

add_executable(test_convergence tests/test_convergence.cpp)
target_link_libraries(test_convergence PRIVATE ope_core)

add_executable(test_implied_vol tests/test_implied_vol.cpp)
target_link_libraries(test_implied_vol PRIVATE ope_core)

add_executable(test_jump_diffusion tests/test_jump_diffusion.cpp)
target_link_libraries(test_jump_diffusion PRIVATE ope_core)

enable_testing()
add_test(NAME black_scholes COMMAND test_black_scholes)
add_test(NAME convergence COMMAND test_convergence)
add_test(NAME implied_vol COMMAND test_implied_vol)
add_test(NAME jump_diffusion COMMAND test_jump_diffusion)

# --- Benchmark (built, not run as a test; timings vary by machine) ---
# Named 'bench' so it does not clash with Google Benchmark's 'benchmark' target.
add_executable(bench bench/benchmark.cpp)
target_link_libraries(bench PRIVATE ope_core)

# --- Optional Google Benchmark suite ---
# Off by default so the normal build and CI stay dependency-free and fast.
# Enable with: cmake -S . -B build -DBUILD_GBENCH=ON ...
option(BUILD_GBENCH "Build the Google Benchmark suite (downloads Google Benchmark)" OFF)
if(BUILD_GBENCH)
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.1)
    FetchContent_MakeAvailable(googlebenchmark)

    add_executable(bench_gb bench/benchmark_gb.cpp)
    target_link_libraries(bench_gb PRIVATE ope_core benchmark::benchmark)
    message(STATUS "Google Benchmark suite enabled -> target 'bench_gb'")
endif()

# --- Optional: Google Benchmark harness (heavier dependency, off by default) ---
# Enable with: cmake -S . -B build -DOPE_BUILD_GBENCH=ON ...
# Kept optional so the default build and CI stay dependency-free and fast.
option(OPE_BUILD_GBENCH "Build the Google Benchmark micro-benchmarks" OFF)
if(OPE_BUILD_GBENCH)
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    FetchContent_MakeAvailable(googlebenchmark)

    add_executable(gbench bench/gbench.cpp)
    target_link_libraries(gbench PRIVATE ope_core benchmark::benchmark)
endif()

# --- Python module via pybind11 (optional: only if pybind11 is found) ---
find_package(pybind11 CONFIG QUIET)
if(pybind11_FOUND)
    pybind11_add_module(ope bindings/bindings.cpp)
    target_link_libraries(ope PRIVATE ope_core)
    message(STATUS "pybind11 found - Python module 'ope' will be built")
else()
    message(STATUS "pybind11 not found - skipping Python module "
                   "(install with: pip install pybind11)")
endif()
