# First-party definitions for docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md.
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)

add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/newgrf/multilayer_industry.grf"
           "${CMAKE_BINARY_DIR}/generated/table/multilayer_content_manifest.h"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/build_content.py"
            --catalogue "${CMAKE_CURRENT_SOURCE_DIR}/cargo_catalogue.json"
            --package "${CMAKE_BINARY_DIR}/newgrf/multilayer_industry.grf"
            --manifest "${CMAKE_BINARY_DIR}/generated/table/multilayer_content_manifest.h"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/build_content.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/cargo_catalogue.json"
    COMMENT "Generating first-party multilayer cargo definitions and exact identity"
    VERBATIM
)

add_custom_target(multilayer_content_files DEPENDS
    "${CMAKE_BINARY_DIR}/newgrf/multilayer_industry.grf"
    "${CMAKE_BINARY_DIR}/generated/table/multilayer_content_manifest.h"
)
add_dependencies(openttd_lib multilayer_content_files)
