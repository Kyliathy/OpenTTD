# Install script for directory: D:/Codez/Sandbox/OpenTTD

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/openttd")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("D:/Codez/Sandbox/OpenTTD/build/bin/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("D:/Codez/Sandbox/OpenTTD/build/src/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("D:/Codez/Sandbox/OpenTTD/build/media/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("D:/Codez/Sandbox/OpenTTD/build/regression/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE EXECUTABLE FILES "D:/Codez/Sandbox/OpenTTD/build/Debug/openttd.exe")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE EXECUTABLE FILES "D:/Codez/Sandbox/OpenTTD/build/Release/openttd.exe")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE EXECUTABLE FILES "D:/Codez/Sandbox/OpenTTD/build/MinSizeRel/openttd.exe")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE EXECUTABLE FILES "D:/Codez/Sandbox/OpenTTD/build/RelWithDebInfo/openttd.exe")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/./newgrf" TYPE FILE FILES "D:/Codez/Sandbox/OpenTTD/build/newgrf/multilayer_industry.grf")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "language_files" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE DIRECTORY FILES
    "D:/Codez/Sandbox/OpenTTD/build/lang"
    "D:/Codez/Sandbox/OpenTTD/build/baseset"
    "D:/Codez/Sandbox/OpenTTD/build/ai"
    "D:/Codez/Sandbox/OpenTTD/build/game"
    "D:/Codez/Sandbox/OpenTTD/bin/scripts"
    REGEX "ai/[^.]+$" EXCLUDE)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "docs" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE FILE FILES
    "D:/Codez/Sandbox/OpenTTD/COPYING.md"
    "D:/Codez/Sandbox/OpenTTD/README.md"
    "D:/Codez/Sandbox/OpenTTD/CREDITS.md"
    "D:/Codez/Sandbox/OpenTTD/CONTRIBUTING.md"
    "D:/Codez/Sandbox/OpenTTD/changelog.md"
    "D:/Codez/Sandbox/OpenTTD/known-bugs.md"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "docs" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/./docs" TYPE FILE FILES
    "D:/Codez/Sandbox/OpenTTD/docs/admin_network.md"
    "D:/Codez/Sandbox/OpenTTD/docs/debugging_desyncs.md"
    "D:/Codez/Sandbox/OpenTTD/docs/desync.md"
    "D:/Codez/Sandbox/OpenTTD/docs/directory_structure.md"
    "D:/Codez/Sandbox/OpenTTD/docs/eints.md"
    "D:/Codez/Sandbox/OpenTTD/docs/fonts.md"
    "D:/Codez/Sandbox/OpenTTD/docs/game_coordinator.md"
    "D:/Codez/Sandbox/OpenTTD/docs/linkgraph.md"
    "D:/Codez/Sandbox/OpenTTD/docs/logging_and_performance_metrics.md"
    "D:/Codez/Sandbox/OpenTTD/docs/multiplayer.md"
    "D:/Codez/Sandbox/OpenTTD/docs/savegame_format.md"
    "D:/Codez/Sandbox/OpenTTD/docs/symbol_server.md"
    "D:/Codez/Sandbox/OpenTTD/docs/obg_format.txt"
    "D:/Codez/Sandbox/OpenTTD/docs/obm_format.txt"
    "D:/Codez/Sandbox/OpenTTD/docs/obs_format.txt"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "D:/Codez/Sandbox/OpenTTD/build/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "D:/Codez/Sandbox/OpenTTD/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
