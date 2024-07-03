import glob, platform, os, pathlib, shutil, subprocess
from Utils import Utils
from Logger import ColoredLogger

class CMakeRequirements:
    __CMakeMinimumVersionRequired = '3.20.0'
    __CMakeVersionPreferred = '3.27.9'

    CMakeChosenPath = ''

    @classmethod
    def Validate(cls) -> bool:
        cmakeExe = 'cmake.exe' if os.name == 'nt' else 'cmake'
        possiblePaths = set()

        # Check if the executable is in the system's PATH
        if shutil.which(cmakeExe) is not None:
            possiblePaths.add(shutil.which(cmakeExe))

        # Check for Windows
        if os.name == 'nt':
            windowsExecutablePath = shutil.which(cmakeExe)
            if windowsExecutablePath:
                possiblePaths.add(windowsExecutablePath)
        else:
            # Check common installation directories for non-Windows platforms
            possibleInstallPaths = [
                '/usr/bin/cmake',           # Linux default
                '/usr/local/bin/cmake',     # Linux alternative
                '/opt/local/bin/cmake',     # macOS default (MacPorts)
                '/usr/local/opt/cmake/bin/cmake',  # macOS default (Homebrew)
                '/Applications/CMake.app/Contents/bin/cmake',  # macOS alternative
            ]

            possiblePaths.update([path for path in possibleInstallPaths if os.path.isfile(path)])
        
        # Search this in case CMake has been downloaded by this script in the previous runs
        localCMakeDir = Utils.GetProjectDirectory() / 'vendor/cmake'
        # This is the search pattern required by the glob.glob function to search for cmake file
        found = glob.glob(str(localCMakeDir / '**' / f'bin/{cmakeExe}'), recursive=True)
        for path in found:
            if pathlib.Path(path).is_file() and os.access(path, os.X_OK):
                possiblePaths.add(path)

        ColoredLogger.Logger.info(f'Found CMake Paths: {possiblePaths}')

        for path in possiblePaths:
            if cls.__ValidateCMakeVersion(path):
                cls.CMakeChosenPath = path
                os.environ['PATH'] += f'{os.pathsep}{pathlib.Path(cls.CMakeChosenPath).parent}'
                ColoredLogger.Logger.info(f'Temporarily added path to shell environment: {path}')
                return True
        
        ColoredLogger.Logger.error('Failed to find CMake!')
        return False

    @classmethod
    def InstallCMake(cls):
        extension = 'zip' if platform.system() == 'Windows' else 'tar.gz'
        URL = cls.__ConstructCMakeURL()

        preferredPath = Utils.GetProjectDirectory() / f'vendor/cmake/CMake.{extension}'
        Utils.DownloadFile(URL, preferredPath)
        Utils.UnzipFile(extension, preferredPath, preferredPath.parent)
        os.remove(preferredPath)

        unzippedPath = "".join(str(preferredPath).split(".")[:-1])
        paths = {
            'Windows': f'{unzippedPath}/bin/cmake.exe',
            'Darwin': f'{unzippedPath}/CMake.app/Contents/bin/cmake',
            'Linux': f'{unzippedPath}/bin/cmake'
        }
        cls.CMakeChosenPath = paths[platform.system()]
        os.environ['PATH'] += f'{os.pathsep}{cls.CMakeChosenPath}'
    
    @classmethod
    def WriteChosenDirectoryToMeta(cls):
        f = open(Utils.GetProjectDirectory() / 'Scripts/Setup.meta', 'w')
        f.write(f'cmake={cls.CMakeChosenPath}\n')
        f.close()

    @classmethod
    def __ValidateCMakeVersion(cls, path) -> bool:
        try:
            output = subprocess.check_output([path, '--version'], stderr=subprocess.STDOUT, universal_newlines=True)
            versionLines = output.splitlines()[:2]  # Extract the first two lines containing version information
            versionInfo = versionLines[0].strip().split()[2]  # Extract version from the last word of the last line
            major, minor, _ = versionInfo.split('.')
            majorR, minorR, _ = cls.__CMakeMinimumVersionRequired.split('.')
            return major >= majorR and minor >= minorR
        except subprocess.CalledProcessError:
            return False
    
    @classmethod
    def __ConstructCMakeURL(cls):
        system = platform.system()
        machine = platform.machine().lower()
        machineName = ''

        if system == 'Windows':
            machineName = 'windows-arm64' if 'arm64' in machine else f'windows-{machine}'
        elif system == 'Darwin':
            machineName = f'macos-universal' if 'arm64' in machine or platform.mac_ver()[0] >= '10.13' else 'macos10.10-universal'
        elif system == 'Linux':
            machineName = f'linux-{machine}'

        extension = 'zip' if system == 'Windows' else 'tar.gz'
        URL = f'https://github.com/Kitware/CMake/releases/download/v{cls.__CMakeVersionPreferred}/cmake-{cls.__CMakeVersionPreferred}-{machineName}.{extension}'
        return URL

if __name__ == '__main__':
    ColoredLogger.InitLogger()
    if not CMakeRequirements.Validate():
        CMakeRequirements.InstallCMake()
    
    CMakeRequirements.WriteChosenDirectoryToMeta()