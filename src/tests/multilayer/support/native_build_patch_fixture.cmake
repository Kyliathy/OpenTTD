# Actual GNU patch fixture seam for the generated provider, with no Git process.
# @file native_build_patch_fixture.cmake Adapter test tooling stays beside its tests.
# @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group13.

#[[/**
 * Provide the existing vcpkg process seam using a real bounded child process.
 * @param ARGV COMMAND structured argv, OUTPUT_FILE generated log, ERROR_VARIABLE
 *             and RESULT_VARIABLE caller bindings, WORKING_DIRECTORY test source.
 * @return Actual error text and exit status through the named caller variables.
 */]]
function(vcpkg_execute_in_download_mode)
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "OUTPUT_FILE;ERROR_VARIABLE;WORKING_DIRECTORY;RESULT_VARIABLE" "COMMAND")
    execute_process(COMMAND ${arg_COMMAND} WORKING_DIRECTORY "${arg_WORKING_DIRECTORY}"
        OUTPUT_FILE "${arg_OUTPUT_FILE}" ERROR_VARIABLE error RESULT_VARIABLE status TIMEOUT 60)
    set(${arg_ERROR_VARIABLE} "${error}" PARENT_SCOPE)
    set(${arg_RESULT_VARIABLE} "${status}" PARENT_SCOPE)
endfunction()

include("${PATCH_HELPER}")
z_vcpkg_apply_patches(SOURCE_PATH "${SOURCE_PATH}" PATCHES "${PATCH_PATH}")
