import os
import time
import pathlib
import requests
import tarfile
import subprocess
from zipfile import ZipFile


def get_project_dir() -> pathlib.Path:
    return pathlib.Path(__file__).parent.parent


def is_valid_git_repo(path: os.PathLike, github_url):
    cwd = os.getcwd()
    # Check if the folder exists
    if not os.path.isdir(path):
        return False
    try:
        os.chdir(path)
        # Check if the remote URL matches the provided GitHub URL
        remote_url = subprocess.check_output(["git", "config", "--get", "remote.origin.url"], universal_newlines=True)
        os.chdir(cwd)
        return github_url in remote_url
    except subprocess.CalledProcessError:
        return False


def download_file(url, file_path):
    if type(url) is not str:
        print("[FLAMEBERRY]: ERROR: URL should be a str!")
        return

    file_name = url.split("/")[-1]

    pathlib.Path(file_path).mkdir(parents=True, exist_ok=True)
    print(f"[FLAMEBERRY]: Preparing to download: {file_name} to path: {file_path}")

    start = time.perf_counter()

    response = requests.get(url, stream=True)
    file_size = int(response.headers.get("content-length"))
    downloaded = 0

    print(f"[FLAMEBERRY]: Downloading! CMake Download Size: {round(file_size / 1024 / 1024, 2)} Mb")

    with open(f"{file_path}/{file_name}", "wb") as f:
        for chunk in response.iter_content(chunk_size=1024 * 1024):
            if chunk:
                f.write(chunk)
                downloaded += len(chunk)
                percent = round(downloaded * 100 / file_size, 2)
                print(f"[FLAMEBERRY]: Downloading CMake... [{percent}%] [{round(downloaded / 1024 / 1024, 3)} MB]")
    print(f"[FLAMEBERRY]: Done! The download took {round(time.perf_counter() - start, 1)} seconds!")


def unzip_file(file_ext, src, dest_folder):
    if file_ext == "zip":
        with ZipFile(src, "r") as zip:
            # extracting all the files
            print(f"[FLAMEBERRY]: Extracting {src}")
            zip.extractall(dest_folder)
    elif file_ext == "tar.gz":
        file = tarfile.open(src)
        file.extractall(dest_folder)
        file.close()
    else:
        print("[FLAMEBERRY]: ERROR: Compressed file extension is not supported!")
        return
    print(f"[FLAMEBERRY]: Extracted {src} to {dest_folder}")
