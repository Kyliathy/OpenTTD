# Development-only shared-owner probes; no game/world sources or Git discovery.
# @file CMakeLists.txt Bounded rebuild for routing and input-identity checks.
# @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group10.
cmake_minimum_required(VERSION 3.21)
project(multilayer_support_probes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(input_sha256_check input_sha256_check.cpp)
add_executable(artifact_paths_probe artifact_paths_probe.cpp ../../test_artifact_paths.cpp)
target_compile_definitions(artifact_paths_probe PRIVATE FMT_HEADER_ONLY)

# Both small probes use the same warning/encoding policy as their actual tested
# owner. These targets do not qualify any application or provisional feature code.
foreach(probe input_sha256_check artifact_paths_probe)
    if(MSVC)
        target_compile_options(${probe} PRIVATE /utf-8 /W4 /WX)
    else()
        target_compile_options(${probe} PRIVATE -Wall -Wextra -Werror)
    endif()
endforeach()
