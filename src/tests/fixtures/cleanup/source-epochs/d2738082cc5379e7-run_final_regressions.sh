#!/usr/bin/env bash
#Use the final build while retaining all output inside the repository.
set -eu
cd /d/Codez/Sandbox/OpenTTD
cmake='/c/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
ctest='/c/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe'
env -u NoDefaultCurrentDirectoryInExePath "$cmake" --build build --config RelWithDebInfo --target regression -- /nodeReuse:false
"$cmake" --build build --config Debug --target regression_files -- /nodeReuse:false
env -u NoDefaultCurrentDirectoryInExePath "$ctest" --test-dir build -C Debug --output-on-failure -R '^regression_gs(_compat)?$'
