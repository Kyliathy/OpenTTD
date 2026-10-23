# Install script for directory: C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/src/v1.3.2-fd3dc4f5e7.clean

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/zlib_x64-windows-static")
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
  set(CMAKE_CROSSCOMPILING "OFF")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/x64-windows-static-rel/zs.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/zlib/ZLIB-static.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/zlib/ZLIB-static.cmake"
         "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/x64-windows-static-rel/CMakeFiles/Export/93f1ef598f1f2f8b07b376ab081bbce6/ZLIB-static.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/zlib/ZLIB-static-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/zlib/ZLIB-static.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/zlib" TYPE FILE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/x64-windows-static-rel/CMakeFiles/Export/93f1ef598f1f2f8b07b376ab081bbce6/ZLIB-static.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/zlib" TYPE FILE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/x64-windows-static-rel/CMakeFiles/Export/93f1ef598f1f2f8b07b376ab081bbce6/ZLIB-static-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/zlib" TYPE FILE FILES
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/x64-windows-static-rel/ZLIBConfig.cmake"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/x64-windows-static-rel/ZLIBConfigVersion.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/x64-windows-static-rel/zconf.h"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/src/v1.3.2-fd3dc4f5e7.clean/zlib.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Docs" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/man/man3" TYPE FILE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/src/v1.3.2-fd3dc4f5e7.clean/zlib.3")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Docs" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/zlib/zlib" TYPE FILE FILES
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/src/v1.3.2-fd3dc4f5e7.clean/LICENSE"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/src/v1.3.2-fd3dc4f5e7.clean/doc/algorithm.txt"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/src/v1.3.2-fd3dc4f5e7.clean/doc/crc-doc.1.0.pdf"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/src/v1.3.2-fd3dc4f5e7.clean/doc/rfc1950.txt"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/src/v1.3.2-fd3dc4f5e7.clean/doc/rfc1951.txt"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/src/v1.3.2-fd3dc4f5e7.clean/doc/rfc1952.txt"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/src/v1.3.2-fd3dc4f5e7.clean/doc/txtvsbin.txt"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/x64-windows-static-rel/zlib.pc")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/x64-windows-static-rel/contrib/cmake_install.cmake")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/x64-windows-static-rel/install_local_manifest.txt"
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
  file(WRITE "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/zlib/x64-windows-static-rel/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
