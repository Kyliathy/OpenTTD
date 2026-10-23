#!/usr/bin/env bash
set -eu
cd /d/Codez/Sandbox/OpenTTD
ctest='/c/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe'
env -u NoDefaultCurrentDirectoryInExePath "$ctest" --test-dir build -C RelWithDebInfo --output-on-failure
