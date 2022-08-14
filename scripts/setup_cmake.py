import glob
import os
import pathlib
import platform
from utils import fl_project_dir, download_file, unzip_file

cmake_found_dirs = []


def setup_cmake() -> bool:
    global cmake_found_dirs
    cmake_search_dirs = [fl_project_dir / 'vendor/cmake']

    if platform.system() == "Darwin":
        cmake_search_dirs.append(pathlib.Path('/usr'))
        cmake_search_dirs.append(pathlib.Path('/Applications'))
    elif platform.system() == "Windows":
        cmake_search_dirs.append(pathlib.Path(f'{os.environ["ProgramFiles"]}'))

    # Setting CMake executable name based on platform
    cmake_exe_name = 'cmake.exe' if platform.system() == 'Windows' else 'cmake'

    print("[FLAMEBERRY]: Searching for CMake...")
    # Searching for CMake locally in vendor/cmake
    for search_dir in cmake_search_dirs:
        # This is the search pattern required by the glob.glob function to search for cmake file
        cmake_path_pattern = str(
            (search_dir / '**' / cmake_exe_name).resolve())

        # found_list is a list of str where cmake is located within the search dir
        found_list = glob.glob(cmake_path_pattern, recursive=True)

        # Appending the paths to cmake_found_dirs after checking if the path points to a valid executable file
        cmake_found_dirs += filter_cmake_paths(found_list)

    if not cmake_found_dirs:
        print('[FLAMEBERRY]: CMake not found. Do you wish to download CMake?(y/n)')
        ans = input('[FLAMEBERRY]: ')
        if ans.lower() == 'y':
            cmake_found_dirs.append(download_cmake())
        else:
            print(
                "[FLAMEBERRY]: That's a No then! Anyways you can manually download CMake from https://cmake.org/download/")
            return False

    for path in cmake_found_dirs:
        print(path + ' - Found')
    return True


def filter_cmake_paths(path_list) -> list[str]:
    if type(path_list) is not list:
        print("[FLAMEBERRY]: Path list must be of type: list and not " +
              str(type(path_list)))
        return []

    filtered_list = []
    for path in path_list:
        error_count = 0
        if pathlib.Path(path).is_file() and os.access(path, os.X_OK):
            if platform.system() == 'Darwin':
                if 'CMake.app/Contents' in path:
                    if 'bin/cmake' not in path:
                        error_count += 1
            elif platform.system() == 'Windows':
                pass

            if error_count == 0:
                filtered_list.append(path)

    return filtered_list


def download_cmake() -> str:
    cmake_version = '3.24.0'

    systemOS = 'macos' if platform.system() == 'Darwin' else platform.system().lower()
    mach = ''
    if systemOS == 'macos':
        mach = 'universal'
    elif platform.machine() == 'AMD64':
        mach = 'x86_64'
    elif platform.machine() == 'i386':
        mach = 'i386'
    elif platform.machine().lower() == 'arm64':
        mach = 'arm64'

    file_extension = 'zip' if systemOS == 'windows' else 'tar.gz'

    url = f'https://github.com/Kitware/CMake/releases/download/v{cmake_version}/cmake-{cmake_version}-{systemOS}-{mach}.{file_extension}'
    print(f'[FLAMEBERRY]: CMake will be downloaded from the URL: {url}')

    download_path = pathlib.Path(fl_project_dir / 'vendor/cmake')
    file_name = str(url.split('/')[-1])

    download_file(url, str(download_path.resolve()))

    src = pathlib.Path(download_path / f'{file_name}')
    pure_filename = file_name.replace(f'.{file_extension}', '')
    dest = download_path

    unzip_file(file_extension, str(src.resolve()), str(dest.resolve()))

    if os.path.exists(str(src.resolve())):
        os.remove(str(src.resolve()))

    cmake_path = str(pathlib.Path(
        dest / f'{pure_filename}/CMake.app/Contents/bin/cmake').resolve()) if systemOS == 'macos' else str(pathlib.Path(dest / f'{pure_filename}/bin/cmake.exe').resolve()) if systemOS == 'windows' else ''
    return cmake_path
