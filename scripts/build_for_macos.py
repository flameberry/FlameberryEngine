import os
import platform
from utils import fl_project_dir
from setup_cmake import setup_cmake, cmake_found_dirs
import subprocess


def setup() -> bool:
    if platform.system() != 'Darwin' and platform.system() != 'Windows':
        print(
            "[FLAMEBERRY]: Flameberry Engine currently only supports Windows and MacOS!")
        return False

    return setup_cmake()


def build_project():
    # Decide CMake path if found multiple instances of CMake
    cmake_path = cmake_found_dirs[0]

    # Determining Build Config for Flameberry Engine
    print("[FLAMEBERRY]: Enter project build configuration('d' for Debug/'r' for Release)")
    build_type = input("[FLAMEBERRY]: ")

    cmake_build_type = 'Release' if build_type.lower() == 'r' else 'Debug'
    print(
        f"[FLAMEBERRY]: {cmake_build_type} configuration selected for building Flameberry Engine.")
    print("[FLAMEBERRY]: Starting CMake Project Generation...")

    # Generating project files using CMake
    subprocess.run(
        [f"{cmake_path}", f"-DCMAKE_BUILD_TYPE={cmake_build_type}", "-Wno-dev", "-S.", "-Bbuild/make"])

    # Building the project using make
    os.chdir(fl_project_dir / 'build/make')

    print("[FLAMEBERRY]: Building Flameberry Engine project.")
    subprocess.run([f"{cmake_path}", '--build', '.'])

    os.chdir(fl_project_dir)
    print(
        f'[FLAMEBERRY]: The Flameberry Engine project executable is generated at the path bin/{cmake_build_type}/SandboxApp-{platform.system()}-{platform.machine()}/SandboxApp_{cmake_build_type}')


if __name__ == '__main__':
    os.chdir(fl_project_dir)
    if setup():
        build_project()
