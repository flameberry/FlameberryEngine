@echo off
pushd %~dp0\..\..\

setlocal enabledelayedexpansion
set "cmakeCommand=cmake.exe"

REM Check if cmake.exe is a valid command
where %cmakeCommand% >nul 2>nul
if %errorlevel% neq 0 (
    set "metaFilePath=.\Build\Scripts\Setup.meta"
    for /f "tokens=1,* delims==" %%a in (%metaFilePath%) do (
        set "%%a=%%b"
    )
    call %cmakeCommand% -Wno-dev -S. -BIntermediate\Build\VisualStudio -G"Visual Studio 17 2022"
) else (
    call cmake.exe -Wno-dev -S. -BIntermediate\Build\VisualStudio -G"Visual Studio 17 2022"
)

popd
PAUSE