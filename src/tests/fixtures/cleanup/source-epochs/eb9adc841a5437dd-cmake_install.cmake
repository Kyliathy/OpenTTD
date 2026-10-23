# Install script for directory: C:/Users/Axonn/AppData/Local/cxc-a

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
  include("C:/Users/Axonn/AppData/Local/cxc-a/build/bin/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Axonn/AppData/Local/cxc-a/build/src/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Axonn/AppData/Local/cxc-a/build/media/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Axonn/AppData/Local/cxc-a/build/regression/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE EXECUTABLE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/Debug/openttd.exe")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE EXECUTABLE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/Release/openttd.exe")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE EXECUTABLE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/MinSizeRel/openttd.exe")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE EXECUTABLE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/RelWithDebInfo/openttd.exe")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/./newgrf" TYPE FILE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/newgrf/multilayer_industry.grf")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "language_files" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE DIRECTORY FILES
    "C:/Users/Axonn/AppData/Local/cxc-a/build/lang"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/baseset"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/ai"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/game"
    "C:/Users/Axonn/AppData/Local/cxc-a/bin/scripts"
    REGEX "ai/[^.]+$" EXCLUDE)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "docs" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE FILE FILES
    "C:/Users/Axonn/AppData/Local/cxc-a/COPYING.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/README.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/CREDITS.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/CONTRIBUTING.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/changelog.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/known-bugs.md"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "docs" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/./docs" TYPE FILE FILES
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/admin_network.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/debugging_desyncs.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/desync.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/directory_structure.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/eints.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/fonts.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/game_coordinator.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/linkgraph.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/logging_and_performance_metrics.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/multiplayer.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/savegame_format.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/symbol_server.md"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/obg_format.txt"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/obm_format.txt"
    "C:/Users/Axonn/AppData/Local/cxc-a/docs/obs_format.txt"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/Users/Axonn/AppData/Local/cxc-a/build/install_local_manifest.txt"
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
  file(WRITE "C:/Users/Axonn/AppData/Local/cxc-a/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
