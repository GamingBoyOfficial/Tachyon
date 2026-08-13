# Copyright (C) 2026 Parikshit Sharma (GamingBoyOfficial)
# This file is part of Tachyon.
# Tachyon is free software under AGPLv3. See LICENSE.

cmake_minimum_required(VERSION 3.25)
project(TachyonEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
if(NOT COMPILER_SUPPORTS_AVX2)
    message(FATAL_ERROR "AVX2 is required for this project.")
endif()

# Aggressive Release flags
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -mtune=native -mavx2 -mfma -DNDEBUG -funroll-loops -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -march=native -mavx2 -mfma")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-unused-parameter")

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

# Header-only library
add_library(tachyon INTERFACE)
target_include_directories(tachyon INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(tachyon INTERFACE cxx_std_23)

find_package(Threads REQUIRED)
target_link_libraries(tachyon INTERFACE Threads::Threads)

# Standalone benchmark
add_executable(tachyon_bench benchmarks/latency_bench.cpp)
target_link_libraries(tachyon_bench PRIVATE tachyon)
target_compile_options(tachyon_bench PRIVATE $<$<CONFIG:Release>:-Ofast>)

# Unit tests
add_executable(tachyon_tests tests/unit_tests.cpp)
target_link_libraries(tachyon_tests PRIVATE tachyon)

# Live capture example
add_executable(tachyon_live examples/live_capture.cpp)
target_link_libraries(tachyon_live PRIVATE tachyon)
target_compile_options(tachyon_live PRIVATE $<$<CONFIG:Release>:-Ofast>)

# Plugin demo example
add_executable(tachyon_plugin_demo examples/plugin_demo.cpp)
target_link_libraries(tachyon_plugin_demo PRIVATE tachyon)
target_compile_options(tachyon_plugin_demo PRIVATE $<$<CONFIG:Release>:-Ofast>)

# Fuzz target (optional)
if(DEFINED ENV{LIB_FUZZER_ENGINE})
    add_executable(tachyon_fuzz fuzz/parser_fuzz.cpp)
    target_link_libraries(tachyon_fuzz PRIVATE tachyon ${LIB_FUZZER_ENGINE})
endif()

# ─── Python bindings (pybind11 auto‑downloaded) ──────────────────────
include(FetchContent)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG v2.11.1
)
FetchContent_MakeAvailable(pybind11)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)

pybind11_add_module(tachyon_core bindings/tachyon_bindings.cpp)
target_link_libraries(tachyon_core PRIVATE tachyon Python3::Module)
target_compile_options(tachyon_core PRIVATE $<$<CONFIG:Release>:-Ofast>)