import os
import pathlib
import glob
import platform
import subprocess

fl_project_dir = pathlib.Path(__file__).parent.parent
fl_project_name = 'FlameberryEngine'


def find_cmake() -> str:
    cmake_search_dir = ''
    preferred_cmake_path = ''

    if platform.system() == 'Darwin':
        preferred_cmake_path = pathlib.Path(
            f'{fl_project_dir}/vendor/cmake/cmake')
        cmake_search_dir = '/usr/**/cmake'
    elif platform.system() == 'Windows':
        preferred_cmake_path = pathlib.Path(
            f'{fl_project_dir}/vendor/cmake/cmake.exe')
        cmake_search_dir = f'{os.environ["ProgramFiles"]}\**\cmake.exe'

    if preferred_cmake_path.is_file():
        print(f"[FLAMEBERRY]: CMake found at {preferred_cmake_path}")
        return preferred_cmake_path

    for path in glob.glob(f'{cmake_search_dir}', recursive=True):
        if path:
            print(f"[FLAMEBERRY]: CMake found at {path}")
            return path

    print("[FLAMEBERRY]: CMake not found!\n[FLAMEBERRY]: If you wish to install cmake, visit https://cmake.org/download/")
    return ''


def build_fl_engine():
    # Searching For CMake
    cmake_path = find_cmake()
    if cmake_path == '':
        return

    # Determining Build Config for Flameberry Engine
    cmake_build_type = 'Debug'
    print("[FLAMEBERRY]: Enter project build configuration('d' for Debug/'r' for Release)")
    build_type = input("[FLAMEBERRY]: ")

    if build_type.lower() == "d":
        cmake_build_type = "Debug"
    elif build_type.lower() == "r":
        cmake_build_type = "Release"

    print(
        f"[FLAMEBERRY]: {cmake_build_type} configuration selected for building Flameberry Engine.")
    print("[FLAMEBERRY]: Starting CMake Project Generation...\n")

    # Generating project files using CMake
    subprocess.run(
        [f"cmake", f"-DCMAKE_BUILD_TYPE={cmake_build_type}", "-Wno-dev", "-S.", "-Bbuild/make"])

    # Building the project using make
    os.chdir(fl_project_dir / 'build/make')

    print("[FLAMEBERRY]: Building Flameberry Engine project using 4 threads.")
    subprocess.run(["make", "-j4"])

    os.chdir(fl_project_dir)
    print(
        f'[FLAMEBERRY]: The Flameberry Engine project executable is generated at the path bin/{cmake_build_type}/SandboxApp-{platform.system()}-{platform.machine()}/SandboxApp_{cmake_build_type}')
