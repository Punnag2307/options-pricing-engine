cmake_minimum_required(VERSION 3.18)
project(options_pricing_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimised by default; this matters once we start benchmarking latency.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# --- Core library (pure C++, no Python dependency) ---
add_library(ope_core STATIC src/black_scholes.cpp)
target_include_directories(ope_core PUBLIC include)
set_target_properties(ope_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- C++ test executable ---
add_executable(test_black_scholes tests/test_black_scholes.cpp)
target_link_libraries(test_black_scholes PRIVATE ope_core)
enable_testing()
add_test(NAME black_scholes COMMAND test_black_scholes)

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
