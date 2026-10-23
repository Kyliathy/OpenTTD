@rem @file compile-input-hash.cmd Disposable native hash verification build.
@rem @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group9.
@call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64 -host_arch=x64
@if errorlevel 1 exit /b 1
@cl /nologo /std:c++20 /EHsc /W4 /WX /Fo"%~dp0input_sha256_check.obj" /Fe"%~dp0input_sha256_check.exe" "%~dp0..\..\src\tests\multilayer\support\input_sha256_check.cpp" /link /INCREMENTAL:NO
