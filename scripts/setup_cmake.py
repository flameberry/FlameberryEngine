import glob
import os
import pathlib
import platform
from utils import get_project_dir, download_file, unzip_file


def setup_cmake() -> list[str]:
    cmake_search_dirs = [get_project_dir() / 'vendor/cmake']

    if platform.system() == "Darwin":
        cmake_search_dirs.append(pathlib.Path('/Applications'))
        cmake_search_dirs.append(pathlib.Path('/usr/local'))
    elif platform.system() == "Windows":
        cmake_search_dirs.append(pathlib.Path(f'{os.environ["LOCALAPPDATA"]}'))
        cmake_search_dirs.append(pathlib.Path(f'{os.environ["PROGRAMFILES"]}'))

    # Setting CMake executable name based on platform
    cmake_exe_name = 'cmake.exe' if platform.system() == 'Windows' else 'cmake'

    print("[FLAMEBERRY]: Searching for CMake...")

    cmake_search_results = []

    # Searching for CMake locally in vendor/cmake
    for search_dir in cmake_search_dirs:
        # This is the search pattern required by the glob.glob function to search for cmake file
        cmake_path_pattern = str((search_dir / '**' / cmake_exe_name).resolve())

        # found_list is a list of str where cmake is located within the search dir
        found_list = glob.glob(cmake_path_pattern, recursive=True)

        # Appending the paths to cmake_search_results after checking if the path points to a valid executable file
        cmake_search_results += filter_cmake_paths(found_list)

    for path in cmake_search_results:
        print('[FLAMEBERRY]: ' + path + ' - Found')
    
    if not cmake_search_results:
        print('[FLAMEBERRY]: CMake not found. Do you wish to download CMake? (y/n)')
        ans = input('[FLAMEBERRY]: ')
        if ans.lower() == 'y':
            downloaded_path = download_cmake()
            cmake_search_results.append(downloaded_path)
        else:
            print("[FLAMEBERRY]: That's a No then! Anyways you can manually download CMake from https://cmake.org/download/")

    return cmake_search_results


def filter_cmake_paths(path_list) -> list[str]:
    if type(path_list) is not list:
        print("[FLAMEBERRY]: ERROR: Path list must be of type: list and not " + str(type(path_list)))
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
    cmake_version = '3.27.1'

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

    download_path = pathlib.Path(get_project_dir() / 'vendor/cmake')
    file_name = str(url.split('/')[-1])

    download_file(url, str(download_path.resolve()))

    downloaded_file = pathlib.Path(download_path / f'{file_name}')
    pure_filename = file_name.replace(f'.{file_extension}', '')
    dest = download_path

    print(f"Extracting {downloaded_file}...")
    unzip_file(file_extension, str(downloaded_file.resolve()), str(dest.resolve()))

    if os.path.exists(downloaded_file):
        os.remove(downloaded_file)

    cmake_path = ''
    if systemOS == 'macos':
        cmake_path = str(pathlib.Path(dest / f'{pure_filename}/CMake.app/Contents/bin/cmake').resolve())
    elif systemOS == 'windows':
        cmake_path = str(pathlib.Path(dest / f'{pure_filename}/bin/cmake.exe').resolve())
    elif systemOS == 'linux':
        cmake_path = str(pathlib.Path(dest / f'{pure_filename}/bin/cmake').resolve())
    return cmake_path
