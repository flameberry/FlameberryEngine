@echo off
pushd %~dp0\..\..\..\

setlocal enabledelayedexpansion
set "cmakeCommand=cmake.exe"

REM Check if cmake.exe is a valid command
where %cmakeCommand% >nul 2>nul
if %errorlevel% neq 0 (
    set "metaFilePath=.\Build\Scripts\Setup.meta"
    for /f "tokens=1,* delims==" %%a in (%metaFilePath%) do (
        set "%%a=%%b"
    )
    call %cmakeCommand% -Wno-dev -DCMAKE_BUILD_TYPE="Release" -S. -BFlameberry\Intermediate\Build\Auto
    call %cmakeCommand% --build Flameberry\Intermediate\Build\Auto
) else (
    call cmake.exe -Wno-dev -DCMAKE_BUILD_TYPE="Release" -S. -BFlameberry\Intermediate\Build\Auto
    call cmake.exe --build Flameberry\Intermediate\Build\Auto
)

echo "Built files are located in the 'bin' folder."
popd
PAUSE
