# @file CMakeLists.txt Isolated generated underground range-contract compilation.
# @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79D.
# @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
# This target compiles the real dry-conversion header without relinking any game.
cmake_minimum_required(VERSION 3.20)
project(GroundHeaderContract LANGUAGES CXX)
get_filename_component(GROUND_REPOSITORY "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
add_library(ground_header_contract OBJECT ground_header_contract.cpp)
target_compile_features(ground_header_contract PRIVATE cxx_std_20)
target_include_directories(ground_header_contract PRIVATE
    "${GROUND_REPOSITORY}/test-cache/ground-pack/conversion-a"
    "${GROUND_REPOSITORY}/src/table")
