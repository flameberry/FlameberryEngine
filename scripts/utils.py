import pathlib
import time
import requests
import tarfile
from zipfile import ZipFile

fl_project_dir = pathlib.Path(__file__).parent.parent
fl_project_name = 'FlameberryEngine'

cmake_search_dirs = []
cmake_found_dirs = []


def download_file(url, file_path):
    if (type(url) is not str):
        print('[FLAMEBERRY]: Url should be a str!')
        return

    file_name = url.split('/')[-1]

    pathlib.Path(file_path).mkdir(parents=True, exist_ok=True)

    print(
        f'[FLAMEBERRY]: Preparing to download: {file_name} to path: {file_path}')

    start = time.perf_counter()

    response = requests.get(url, stream=True)
    file_size = int(response.headers.get('content-length'))
    downloaded = 0

    print(
        f'[FLAMEBERRY]: Downloading! CMake Download Size: {round(file_size * 0.000001, 2)} Mb')

    with open(f'{file_path}/{file_name}', 'wb') as f:
        for chunk in response.iter_content(chunk_size=1024 * 1024):
            if chunk:
                downloaded += len(chunk)
                f.write(chunk)
    print(
        f'[FLAMEBERRY]: Done! The download took {round(time.perf_counter() - start, 1)} seconds!')


def unzip_file(file_ext, src, dest_folder):
    if file_ext == 'zip':
        with ZipFile(src, 'r') as zip:
            # extracting all the files
            print(f'[FLAMEBERRY]: Extracting {src}')
            zip.extractall(dest_folder)
    elif file_ext == 'tar.gz':
        file = tarfile.open(src)
        file.extractall(dest_folder)
        file.close()
    else:
        print('[FLAMEBERRY]: Following compressed file extension is not supported!')
        return
    print(f'[FLAMEBERRY]: Extracted {src} to {dest_folder}')
