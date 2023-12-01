@echo off

pushd %~dp0\..\
call vendor\cmake\cmake.exe -Wno-dev -DCMAKE_BUILD_TYPE=Release -S . -B build\MinGW-Makefiles -G "MinGW Makefiles"
cd build\MinGW-Makefiles
call vendor\cmake\cmake.exe --build .
popd
echo "[FLAMEBERRY]: The executable is generated in the 'bin' directory in the main project folder"

PAUSE
