@echo off

pushd %~dp0\..\
call vendor\cmake\cmake.exe -Wno-dev -DCMAKE_BUILD_TYPE=Release -S . -B build\make\
cd build\make
make -j4
popd
echo "[FLAMEBERRY]: The executable is generated in the 'bin' directory in the main project folder"

PAUSE
