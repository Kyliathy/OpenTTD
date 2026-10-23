# Disposable provider adapter; pinned source patches use GNU patch without Git writes.
# @file native_build_apply_patches.cmake.in Only provider materialization substitutes the tool path.
# @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group13.

#[[/**
 * Apply the selected port's original patches inside its generated source owner.
 * @param ARGV CMake keyword arguments: SOURCE_PATH generated extracted source,
 *             PATCHES original port patch names, and QUIET for optional probes.
 * @return No value; propagate a failed mandatory patch through CMake.
 */]]
function(z_vcpkg_apply_patches)
    cmake_parse_arguments(PARSE_ARGV 0 arg "QUIET" "SOURCE_PATH" "PATCHES")
    if(DEFINED arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unsupported pinned patch arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()
    set(patch_executable [==[C:/Program Files/Git/usr/bin/patch.exe]==])
    set(patchnum 0)
    # Each original port patch belongs to this extracted build source. Apply in
    # declared order; a failure cannot silently qualify a differently built library.
    foreach(patch IN LISTS arg_PATCHES)
        get_filename_component(absolute_patch "${patch}" ABSOLUTE BASE_DIR "${CURRENT_PORT_DIR}")
        set(logname "patch-${TARGET_TRIPLET}-${patchnum}")
        vcpkg_execute_in_download_mode(
            COMMAND "${patch_executable}" -p1 --binary --batch --forward --ignore-whitespace
                    --no-backup-if-mismatch --input "${absolute_patch}"
                    --reject-file "${CURRENT_BUILDTREES_DIR}/${logname}.rej"
            OUTPUT_FILE "${CURRENT_BUILDTREES_DIR}/${logname}-out.log"
            ERROR_VARIABLE error WORKING_DIRECTORY "${arg_SOURCE_PATH}"
            RESULT_VARIABLE error_code
        )
        file(WRITE "${CURRENT_BUILDTREES_DIR}/${logname}-err.log" "${error}")
        if(error_code)
            if(arg_QUIET)
                message(STATUS "Optional source patch failed: ${patch}")
            else()
                message(FATAL_ERROR "Pinned source patch failed: ${patch}: ${error}")
            endif()
        endif()
        math(EXPR patchnum "${patchnum} + 1")
    endforeach()
endfunction()
