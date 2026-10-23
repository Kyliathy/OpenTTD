@rem @file artifact_paths_probe.cmd Compile the actual routing owner without the game.
@rem @see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group25.
@rem Arguments: none. Requires the observed Visual Studio 2022 BuildTools install.
@rem All generated objects/images go under this checkout's test-cache/cleanup.
@setlocal
@set "probe_output=%~dp0..\..\..\..\test-cache\cleanup"
@if not exist "%probe_output%" mkdir "%probe_output%"
@call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64 -host_arch=x64
@if errorlevel 1 exit /b 1
@cl /nologo /std:c++20 /utf-8 /EHsc /W4 /WX /MT /DFMT_HEADER_ONLY /c /Fo"%probe_output%\test_artifact_paths.obj" "%~dp0..\..\test_artifact_paths.cpp"
@if errorlevel 1 exit /b 1
@cl /nologo /std:c++20 /utf-8 /EHsc /W4 /WX /MT /c /Fo"%probe_output%\artifact_paths_probe.obj" "%~dp0artifact_paths_probe.cpp"
@if errorlevel 1 exit /b 1
@link /nologo /INCREMENTAL:NO /OUT:"%probe_output%\artifact_paths_probe.exe" "%probe_output%\test_artifact_paths.obj" "%probe_output%\artifact_paths_probe.obj"
@exit /b %errorlevel%
