@echo off
cls
setlocal

:: ----------------------------------------------------------------------------------
set name=Pong
set project_root=..\..
set build_dir=%project_root%\build\win32
:: ----------------------------------------------------------------------------------

set common_compiler_flags=/D "PROGRAM_NAME=\"%name%\""^
                          /EHa-s-c-^
                          /fp:except-^
                          /fp:fast^
                          /GR-^
                          /GS-^
                          /Gs9999999^
                          /Zl^
                          /W4

set common_linker_flags=/link^
                        /NODEFAULTLIB^
                        /STACK:0x100000,0x100000^
                        /SUBSYSTEM:WINDOWS^
                        /WX^
                        /INCREMENTAL:NO^
                        /OPT:REF^
                        /MACHINE:X64

set common_compiler_dev_flags=/D DEVELOPMENT /Z7 /Zo
set common_compiler_slow_flags=%common_compiler_dev_flags% /D SLOW /Od /Oi
set common_compiler_fast_flags=%common_compiler_dev_flags% /D FAST /O2
set common_compiler_release_flags=/D RELEASE /O2
set common_linker_dev_flags=/OPT:NOICF
set common_linker_release_flags=/OPT:ICF

:: ----------------------------------------------------------------------------------

set clang_version=12.0.1
set clang_include_dir=%ProgramFiles%\LLVM\lib\clang\%clang_version%\include

set clang_disabled_errors=-Wno-error=unused-variable^
                          -Wno-error=unused-function^
                          -Wno-error=unused-parameter^
                          -Wno-error=unused-macros

set clang_disabled_warnings=-Wno-reserved-id-macro^
                            -Wno-gnu-include-next^
                            -Wno-extra-semi-stmt^
                            -Wno-double-promotion^
                            -Wno-unreachable-code^
                            -Wno-unreachable-code-break

set clang_common_compiler_flags=-nobuiltininc^
                                /clang:-std=c99^
                                /clang:-fuse-ld=lld^
                                /WX^
                                /I"%clang_include_dir%"^
                                %common_compiler_flags%^
                                %clang_disabled_errors%^
                                %clang_disabled_warnings%

:: ----------------------------------------------------------------------------------

set msvc_disabled_warnings=/wd4464 /wd4668 /wd5045 /wd4711 /wd4710 /wd4706

set msvc_common_compiler_flags=/nologo^
                               /D MSVC^
                               /WL^
                               /Gm-^
                               /diagnostics:column^
                               /MP^
                               %common_compiler_flags%^
                               %msvc_disabled_warnings%

:: ----------------------------------------------------------------------------------

set source_files=win32_main.c

set libraries=kernel32.lib^
              user32.lib^
              gdi32.lib^
              winmm.lib^
              ole32.lib

:: ==================================================================================

where clang-cl >NUL 2>NUL
IF %ERRORLEVEL% NEQ 0 (
    echo clang-cl not found. Make sure LLVM is installed and in the PATH.
) && goto skip_clang

echo Building Clang x64 Slow
echo -----------------------

set build_config=clang_x64_slow
set output_dir=%build_dir%\%build_config%
set executable=%output_dir%\%name%_%build_config%.exe

IF NOT EXIST %output_dir% (mkdir %output_dir%)
del /Q %output_dir%\* >NUL

clang-cl^
 %clang_common_compiler_flags%^
 %common_compiler_slow_flags%^
 /Fe"%executable%"^
 %source_files%^
 %libraries%^
 %common_linker_flags%^
 %common_linker_dev_flags%

echo.
echo.
echo.
echo Building Clang x64 Fast
echo -----------------------

set build_config=clang_x64_fast
set output_dir=%build_dir%\%build_config%
set executable=%output_dir%\%name%_%build_config%.exe

IF NOT EXIST %output_dir% (mkdir %output_dir%)
del /Q %output_dir%\* >NUL

clang-cl^
 %clang_common_compiler_flags%^
 %common_compiler_fast_flags%^
 /Fe"%executable%"^
 %source_files%^
 %libraries%^
 %common_linker_flags%^
 %common_linker_dev_flags%

echo.
echo.
echo.
echo Building Clang x64 Release
echo --------------------------

set build_config=clang_x64_release
set output_dir=%build_dir%\%build_config%
set executable=%output_dir%\%name%_%build_config%.exe

IF NOT EXIST %output_dir% (mkdir %output_dir%)
del /Q %output_dir%\* >NUL

clang-cl^
 %clang_common_compiler_flags%^
 %common_compiler_release_flags%^
 /Fe"%executable%"^
 %source_files%^
 %libraries%^
 %common_linker_flags%^
 %common_linker_release_flags%

:skip_clang

:: ==================================================================================

echo.
echo.
echo.
echo.
echo.

IF "%VCToolsVersion%"=="" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >NUL
    set
    IF "%VCToolsVersion%"=="" (
        echo cl not found. Make sure MSVC is installed and 'vcvarsall.bat x64' executed.
        goto skip_msvc
    )
)

echo Building MSVC x64 Slow
echo ----------------------

set build_config=msvc_x64_slow
set output_dir=%build_dir%\%build_config%
set executable=%output_dir%\%name%_%build_config%.exe

IF NOT EXIST %output_dir% (mkdir %output_dir%)
del /Q %output_dir%\* >NUL

cl^
 %msvc_common_compiler_flags%^
 %common_compiler_slow_flags%^
 /Fe"%executable%"^
 %source_files%^
 %libraries%^
 %common_linker_flags%^
 %common_linker_dev_flags%

echo.
echo.
echo.
echo Building MSVC x64 Fast
echo ----------------------

set build_config=msvc_x64_fast
set output_dir=%build_dir%\%build_config%
set executable=%output_dir%\%name%_%build_config%.exe

IF NOT EXIST %output_dir% (mkdir %output_dir%)
del /Q %output_dir%\* >NUL

cl^
 %msvc_common_compiler_flags%^
 %common_compiler_fast_flags%^
 /Fe"%executable%"^
 %source_files%^
 %libraries%^
 %common_linker_flags%^
 %common_linker_dev_flags%

echo.
echo.
echo.
echo Building MSVC x64 Release
echo -------------------------

set build_config=msvc_x64_release
set output_dir=%build_dir%\%build_config%
set executable=%output_dir%\%name%_%build_config%.exe

IF NOT EXIST %output_dir% (mkdir %output_dir%)
del /Q %output_dir%\* >NUL

cl^
 %msvc_common_compiler_flags%^
 %common_compiler_release_flags%^
 /Fe"%executable%"^
 %source_files%^
 %libraries%^
 %common_linker_flags%^
 %common_linker_release_flags%

del *.obj
echo.
:skip_msvc

:: ==================================================================================

echo.
echo.
echo.
echo.
echo.
echo Checking Git Status
echo -------------------

git status

echo.
echo.
echo.
echo.
echo.
echo Counting Lines of Code
echo ----------------------

cloc --exclude-dir=third_party %project_root%\.

echo.
