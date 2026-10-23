@rem @file compile-artifact-paths.cmd Disposable actual routing owner build.
@rem @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group10.
@call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64 -host_arch=x64
@if errorlevel 1 exit /b 1
@cl /nologo /std:c++20 /utf-8 /EHsc /W4 /WX /MT /DFMT_HEADER_ONLY /c /Fo"%~dp0test_artifact_paths.obj" "%~dp0..\..\src\tests\test_artifact_paths.cpp"
@if errorlevel 1 exit /b 1
@cl /nologo /std:c++20 /utf-8 /EHsc /W4 /WX /MT /c /Fo"%~dp0artifact_paths_probe.obj" "%~dp0..\..\src\tests\multilayer\support\artifact_paths_probe.cpp"
@if errorlevel 1 exit /b 1
@link /nologo /INCREMENTAL:NO /OUT:"%~dp0artifact_paths_probe.exe" "%~dp0test_artifact_paths.obj" "%~dp0artifact_paths_probe.obj"
