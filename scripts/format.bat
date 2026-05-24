@echo off
rem Apply clang-format -i to every C/C++ source under inc/ and src/.
rem inc/opcodes.hpp is auto-skipped via the // clang-format off marker at its top.
rem
rem Usage:
rem   scripts\format.bat            run from anywhere — resolves repo root from %~dp0
rem
rem Resolution order for the clang-format executable:
rem   1. clang-format on PATH
rem   2. clang-format bundled with Visual Studio 2022 (Community/Pro/Enterprise/BuildTools)

setlocal EnableDelayedExpansion

pushd "%~dp0.." >nul

set "CLANG_FORMAT="
where clang-format >nul 2>nul && set "CLANG_FORMAT=clang-format"

if not defined CLANG_FORMAT (
    for %%E in (Community Professional Enterprise BuildTools) do (
        set "CANDIDATE=%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Tools\Llvm\bin\clang-format.exe"
        if not defined CLANG_FORMAT if exist "!CANDIDATE!" set "CLANG_FORMAT=!CANDIDATE!"
    )
)

if not defined CLANG_FORMAT (
    echo [ERROR] clang-format not found on PATH or under "%ProgramFiles%\Microsoft Visual Studio\2022\*\VC\Tools\Llvm\bin\".
    popd >nul
    exit /b 1
)

echo Using: !CLANG_FORMAT!
echo Formatting inc\ and src\ ...

set /a COUNT=0
for /r inc %%F in (*.h *.hpp *.hh *.hxx *.inl *.ipp) do (
    "!CLANG_FORMAT!" -i -style=file --fallback-style=none "%%F" || goto :fail
    set /a COUNT+=1
)
for /r src %%F in (*.c *.cc *.cpp *.cxx *.h *.hpp) do (
    "!CLANG_FORMAT!" -i -style=file --fallback-style=none "%%F" || goto :fail
    set /a COUNT+=1
)

echo Done. Formatted !COUNT! files.
popd >nul
exit /b 0

:fail
echo [ERROR] clang-format failed on "%%F".
popd >nul
exit /b 1
