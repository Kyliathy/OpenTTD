# Install script for directory: C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static")
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

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/share/doc/lzo/AUTHORS;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/share/doc/lzo/COPYING;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/share/doc/lzo/NEWS;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/share/doc/lzo/THANKS;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/share/doc/lzo/LZO.FAQ;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/share/doc/lzo/LZO.TXT;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/share/doc/lzo/LZOAPI.TXT")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/share/doc/lzo" TYPE FILE FILES
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/AUTHORS"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/COPYING"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/NEWS"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/THANKS"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/doc/LZO.FAQ"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/doc/LZO.TXT"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/doc/LZOAPI.TXT"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo/lzo1.h;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo/lzo1a.h;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo/lzo1b.h;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo/lzo1c.h;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo/lzo1f.h;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo/lzo1x.h;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo/lzo1y.h;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo/lzo1z.h;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo/lzo2a.h;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo/lzo_asm.h;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo/lzoconf.h;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo/lzodefs.h;C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo/lzoutil.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/include/lzo" TYPE FILE FILES
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/include/lzo/lzo1.h"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/include/lzo/lzo1a.h"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/include/lzo/lzo1b.h"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/include/lzo/lzo1c.h"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/include/lzo/lzo1f.h"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/include/lzo/lzo1x.h"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/include/lzo/lzo1y.h"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/include/lzo/lzo1z.h"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/include/lzo/lzo2a.h"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/include/lzo/lzo_asm.h"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/include/lzo/lzoconf.h"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/include/lzo/lzodefs.h"
    "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/src/lzo-2-a4047e1779.clean/include/lzo/lzoutil.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/lib/lzo2.lib")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/lib" TYPE STATIC_LIBRARY FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/x64-windows-static-rel/lzo2.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/libexec/lzo/examples/lzopack.exe")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/libexec/lzo/examples" TYPE EXECUTABLE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/x64-windows-static-rel/lzopack.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/x64-windows-static-rel/CMakeFiles/lzopack.dir/install-cxx-module-bmi-Release.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/libexec/lzo/examples/lzotest.exe")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/libexec/lzo/examples" TYPE EXECUTABLE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/x64-windows-static-rel/lzotest.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/x64-windows-static-rel/CMakeFiles/lzotest.dir/install-cxx-module-bmi-Release.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/libexec/lzo/examples/simple.exe")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/libexec/lzo/examples" TYPE EXECUTABLE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/x64-windows-static-rel/simple.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/x64-windows-static-rel/CMakeFiles/simple.dir/install-cxx-module-bmi-Release.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/libexec/lzo/examples/testmini.exe")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/libexec/lzo/examples" TYPE EXECUTABLE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/x64-windows-static-rel/testmini.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/x64-windows-static-rel/CMakeFiles/testmini.dir/install-cxx-module-bmi-Release.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/lib/pkgconfig/lzo2.pc")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/packages/lzo_x64-windows-static/lib/pkgconfig" TYPE FILE FILES "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/x64-windows-static-rel/lzo2.pc")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/x64-windows-static-rel/install_local_manifest.txt"
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
  file(WRITE "C:/Users/Axonn/AppData/Local/cxc-a/build/.native-work/buildtrees/lzo/x64-windows-static-rel/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
