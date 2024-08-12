import os, time, pathlib, requests, tarfile, subprocess, re
from zipfile import ZipFile
from Logger import ColoredLogger
from requests.exceptions import RequestException


class Utils:
    @classmethod
    def GetProjectDirectory(cls) -> pathlib.Path:
        return pathlib.Path(__file__).parent.parent.parent.parent

    @classmethod
    def GetVisualStudioVersionIfInstalled(cls):
        try:
            # Run the reg command to query the Windows Registry for Visual Studio installation
            result = subprocess.run(
                ["reg", "query", "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio"],
                capture_output=True,
                text=True,
            )

            # Check if the command was successful (return code 0) and if Visual Studio keys are present in the output
            if result.returncode == 0 and "VisualStudio" in result.stdout:
                # Extract version information from the output using regular expressions
                versionMatch = re.search(r"VisualStudio\\(\d+)", result.stdout)
                if versionMatch:
                    return versionMatch.group(1)
            return None

        except Exception as e:
            ColoredLogger.Logger.error(e)
            return None

    @classmethod
    def IsXcodeInstalled(cls):
        try:
            # Run the xcode-select command to check if Xcode is installed
            result = subprocess.run(["xcode-select", "--print-path"], capture_output=True, text=True)

            # Check if the command was successful (return code 0) and if Xcode path is present in the output
            return result.returncode == 0 and "/Applications/Xcode.app" in result.stdout.strip()

        except Exception as e:
            ColoredLogger.Logger.error(e)
            return False

    @classmethod
    def IsValidGitRepository(cls, path: os.PathLike, githubURL):
        cwd = os.getcwd()
        # Check if the folder exists
        if not os.path.isdir(path):
            return False
        try:
            os.chdir(path)
            # Check if the remote URL matches the provided GitHub URL
            remoteURL = subprocess.check_output(
                ["git", "config", "--get", "remote.origin.url"], universal_newlines=True
            )
            os.chdir(cwd)
            return githubURL in remoteURL
        except subprocess.CalledProcessError:
            return False

    @classmethod
    def PrintProgressBar(cls, iteration, total, prefix="", suffix="", decimals=1, length=100, fill="█", printEnd="\r"):
        """
        Call in a loop to create terminal progress bar
        @params:
            iteration   - Required  : current iteration (Int)
            total       - Required  : total iterations (Int)
            prefix      - Optional  : prefix string (Str)
            suffix      - Optional  : suffix string (Str)
            decimals    - Optional  : positive number of decimals in percent complete (Int)
            length      - Optional  : character length of bar (Int)
            fill        - Optional  : bar fill character (Str)
        """
        percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
        filledLength = int(length * iteration // total)
        bar = fill * filledLength + "-" * (length - filledLength)
        print(f"\r{prefix} |{bar}| {percent}% {suffix}", end=printEnd)

        # Print New Line on Complete
        if iteration == total:
            print()

    @classmethod
    def DownloadFile(cls, URL, filepath: os.PathLike) -> bool:
        if type(URL) is not str:
            ColoredLogger.Logger.error("Download URL should be a str!")
            return False

        filename = str(filepath).split("/")[-1]
        filepath = os.path.abspath(filepath)
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        ColoredLogger.Logger.info(f"Preparing to download: {URL} to path: {filepath}")

        try:
            response = requests.get(URL, stream=True)
            response.raise_for_status()
            total = int(response.headers.get("content-length", 0))
        except RequestException as e:
            ColoredLogger.Logger.error(f"Error downloading {filename}: {e}")
            return False

        if total == 0:
            ColoredLogger.Logger.warning("Server did not provide Content-Length header.")
        ColoredLogger.Logger.info(f"{filename} Download Size: {total / 1024 / 1024:.2f} Mb")

        downloaded = 0
        with open(filepath, "wb") as f:
            startTime = time.perf_counter()
            for chunk in response.iter_content(chunk_size=max(total / 1000, 1024 * 1024)):
                if chunk:
                    f.write(chunk)
                    downloaded += len(chunk)

                    elapsedTime = time.perf_counter() - startTime
                    avgKBPS = (downloaded / 1024) / elapsedTime
                    speedString = f"{avgKBPS:.2f} KB/s"
                    if avgKBPS > 1024:
                        avgMBPS = avgKBPS / 1024
                        speedString = f"{avgMBPS:.2f} MB/s"

                    suffix = f"({downloaded / 1024 / 1024:.2f} / {total / 1024 / 1024:.2f} MB) (Speed: {speedString})"
                    try:
                        width = os.get_terminal_size().columns - 50
                        printEnd = "\r"
                    except OSError:
                        width = 75
                        printEnd = ""

                    cls.PrintProgressBar(downloaded, total, length=width, suffix=suffix, printEnd=printEnd)

        if total != downloaded:
            ColoredLogger.Logger.error(f"Failed to download {filename}!")
            return False
        return True

    @classmethod
    def UnzipFile(cls, extension: str, src, dest):
        if extension == "zip":
            with ZipFile(src, "r") as zip:
                ColoredLogger.Logger.info(f"Extracting {src}")
                zip.extractall(dest)
        elif extension.startswith("tar."):
            file = tarfile.open(src)
            file.extractall(dest)
            file.close()
        else:
            ColoredLogger.Logger.error("Compressed file extension is not supported!")
            return
        ColoredLogger.Logger.info(f"Done!")
