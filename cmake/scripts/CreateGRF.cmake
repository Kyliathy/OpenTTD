cmake_minimum_required(VERSION 3.17)

#
# Create a single GRF file based on sprites/<grfname>.nfo and sprites/*.png
# files.
# @see docs/zz-reach2/upgrades/2026-10/r2mlm6-layer-graphics-assets.md
#

if(NOT NFORENUM_EXECUTABLE)
    message(FATAL_ERROR "Script needs NFORENUM_EXECUTABLE defined")
endif()
if(NOT GRFCODEC_EXECUTABLE)
    message(FATAL_ERROR "Script needs GRFCODEC_EXECUTABLE defined")
endif()
if(NOT GRFID_EXECUTABLE)
    message(FATAL_ERROR "Script needs GRFID_EXECUTABLE defined")
endif()
if(NOT GRF_SOURCE_FOLDER)
    message(FATAL_ERROR "Script needs GRF_SOURCE_FOLDER defined")
endif()
if(NOT GRF_BINARY_FILE)
    message(FATAL_ERROR "Script needs GRF_BINARY_FILE defined")
endif()

# Remove the existing output so failures never go unnoticed
file(REMOVE ${GRF_BINARY_FILE} ${GRF_BINARY_FILE}.hash)

get_filename_component(GRF_SOURCE_FOLDER_NAME "${GRF_SOURCE_FOLDER}" NAME)

file(WRITE sprites/${GRF_SOURCE_FOLDER_NAME}.nfo "")
file(READ ${GRF_SOURCE_FOLDER}/${GRF_SOURCE_FOLDER_NAME}.nfo NFO_LINES)
# Replace ; with \;, and make a list out of this based on \n
string(REPLACE ";" "\\;" NFO_LINES "${NFO_LINES}")
string(REPLACE "\n" ";" NFO_LINES "${NFO_LINES}")

foreach(NFO_LINE IN LISTS NFO_LINES)
    # Recover the ; that was really in the text (and not a newline)
    string(REPLACE "\\;" ";" NFO_LINE "${NFO_LINE}")

    if(NFO_LINE MATCHES "^#include")
        string(REGEX REPLACE "^#include \"(.*)\"$" "\\1" INCLUDE_FILE ${NFO_LINE})
        file(READ ${GRF_SOURCE_FOLDER}/${INCLUDE_FILE} INCLUDE_LINES)
        file(APPEND sprites/${GRF_SOURCE_FOLDER_NAME}.nfo "${INCLUDE_LINES}")
    else()
        file(APPEND sprites/${GRF_SOURCE_FOLDER_NAME}.nfo "${NFO_LINE}\n")
    endif()
endforeach()

# NFORenum 6.2.0 on Windows needs its embedded data materialised before linting.
# Keep that cache beside the generated sprites instead of writing to the user profile.
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/.nforenum/5.dat")
    execute_process(COMMAND ${NFORENUM_EXECUTABLE} "--data=${CMAKE_CURRENT_BINARY_DIR}" --write-data
        OUTPUT_QUIET ERROR_VARIABLE DATA_ERROR RESULT_VARIABLE RESULT)
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to initialise NFORenum data (${RESULT}): ${DATA_ERROR}")
    endif()
endif()

execute_process(COMMAND ${NFORENUM_EXECUTABLE} "--data=${CMAKE_CURRENT_BINARY_DIR}" -s sprites/${GRF_SOURCE_FOLDER_NAME}.nfo
    OUTPUT_VARIABLE NFORENUM_OUTPUT RESULT_VARIABLE RESULT)
if(RESULT)
    message(STATUS "${NFORENUM_OUTPUT}")
    if(NOT RESULT MATCHES "^[0-9]*$")
        message(FATAL_ERROR "Failed to run NFORenum (${RESULT}), please check NFORENUM_EXECUTABLE variable")
    endif()
    message(FATAL_ERROR "NFORenum failed")
endif()

execute_process(COMMAND ${GRFCODEC_EXECUTABLE} -n -s -e -g2 -p1 ${GRF_SOURCE_FOLDER_NAME}.grf RESULT_VARIABLE RESULT)
if(RESULT)
    if(NOT RESULT MATCHES "^[0-9]*$")
        message(FATAL_ERROR "Failed to run GRFCodec (${RESULT}), please check GRFCODEC_EXECUTABLE variable")
    endif()
    message(FATAL_ERROR "GRFCodec failed")
endif()

execute_process(COMMAND ${GRFID_EXECUTABLE} -m ${GRF_SOURCE_FOLDER_NAME}.grf OUTPUT_VARIABLE GRFID_HASH RESULT_VARIABLE RESULT)
if(RESULT)
    if(NOT RESULT MATCHES "^[0-9]*$")
        message(FATAL_ERROR "Failed to run GRFID (${RESULT}), please check GRFID_EXECUTABLE variable")
    endif()
    message(FATAL_ERROR "GRFID failed")
endif()

file(WRITE ${GRF_BINARY_FILE}.hash ${GRFID_HASH})

# Copy build files back to the source directory.
execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${GRF_SOURCE_FOLDER_NAME}.grf ${GRF_BINARY_FILE})
