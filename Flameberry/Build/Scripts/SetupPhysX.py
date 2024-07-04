import os
import platform
import subprocess
import glob
import pathlib
from Logger import ColoredLogger
from Utils import Utils


class PhysXSDKRequirements:
    __PhysXRootDirectory = Utils.GetProjectDirectory(
    ) / 'Flameberry/Source/ThirdParty/PhysX'
    __ShouldTakePresetFromUser = True

    @classmethod
    def Validate(cls):
        innerBinDir = cls.__GetValidBuiltLibraryDirectory()
        if innerBinDir is None:
            ColoredLogger.Logger.warning(
                f'Nvidia PhysX libraries not found in {cls.__PhysXRootDirectory}')
            return False
        ColoredLogger.Logger.info(
            f'Found PhysX libraries in the directory: {innerBinDir}')
        return True

    @classmethod
    def BuildPhysXSDK(cls):
        # Find a PhysX build preset
        preset = '' if cls.__ShouldTakePresetFromUser else cls.__FindPreset(
            cls.__PhysXRootDirectory / 'physx/buildtools/presets/public')

        if preset != '':
            presetParam = [preset]
        else:
            presetParam = []

        cls.__UpdatePackman()

        try:
            cwd = os.getcwd()
            os.chdir(cls.__PhysXRootDirectory / 'physx')

            # Generate PhysX Project Files
            if os.name == 'nt':
                params = [cls.__PhysXRootDirectory /
                          'physx/generate_projects.bat'] + presetParam
                subprocess.run(params)
            else:
                params = ['sh', cls.__PhysXRootDirectory /
                          'physx/generate_projects.sh'] + presetParam
                subprocess.run(params)

            # Build PhysX Libraries
            compilerDir = cls.__PhysXRootDirectory / 'physx/compiler'
            innerCompilerDirs = [x for x in os.listdir(compilerDir) if (
                compilerDir / x).is_dir() and preset in x]

            # Only relevant if preset is empty (selected by user in the previously called script)
            if preset == '':
                innerCompilerDirs.remove('public')

            for dir in innerCompilerDirs:
                # Build for multi-config generators like GNU Make
                if 'checked' in dir or 'release' in dir:
                    subprocess.run(["cmake", "--build", compilerDir / dir])
                    continue

                if 'debug' not in dir and 'profile' not in dir:
                    # Build for single-config generators like Xcode, Visual Studio
                    for config in ('checked', 'release'):
                        subprocess.run(
                            ["cmake", "--build", compilerDir / dir, f"--config={config}"])

            os.chdir(cwd)
        except Exception as e:
            raise e

        # Write the newly built PhysX library paths
        innerBinDir = cls.__GetValidBuiltLibraryDirectory()
        includeDir = cls.__PhysXRootDirectory / 'physx/include'

        outputString = f'''
# Following code is generated by the generate_project.py script

set(PHYSX_INCLUDE_DIR "${{FBY_SOURCE_DIR}}/{includeDir.relative_to(Utils.GetProjectDirectory())}")
set(PHYSX_CHECKED_LIB_DIRECTORY "${{FBY_SOURCE_DIR}}/{innerBinDir.relative_to(Utils.GetProjectDirectory())}/checked")
set(PHYSX_RELEASE_LIB_DIRECTORY "${{FBY_SOURCE_DIR}}/{innerBinDir.relative_to(Utils.GetProjectDirectory())}/release")
set(PHYSX_COMPILE_DEFINITIONS NDEBUG)
'''
        cmakeEnvPhysXFile = open(Utils.GetProjectDirectory(
        ) / "Flameberry/Build/CMake/envPhysX.cmake", "w")
        cmakeEnvPhysXFile.write(outputString)
        cmakeEnvPhysXFile.close()

    @classmethod
    def __UpdatePackman(cls):
        cwd = os.getcwd()
        os.chdir(cls.__PhysXRootDirectory / 'physx/buildtools/packman')

        try:
            packman = '.\\packman.cmd' if os.name == 'nt' else './packman'
            params = [packman, 'update', '-y', '--major']
            subprocess.run(params)
        except Exception as e:
            raise e

        os.chdir(cwd)

    @classmethod
    def __FindPreset(cls, parent):
        if platform.system() == 'Windows':
            visualStudioVersion = Utils.GetVisualStudioVersionIfInstalled()
            if visualStudioVersion is None:
                ColoredLogger.Logger.error(
                    'Visual Studio must be installed for PhysX to be built')
                quit()

        elif platform.system() == 'Darwin':
            isXcodeInstalled = Utils.IsXcodeInstalled()

        for file in glob.glob(str(parent / "*.xml")):
            filepath = pathlib.Path(file)
            presetName = filepath.name.removesuffix(".xml")

            # Choose preset based upon platform
            if platform.system() == "Windows":
                if "win64" and f'vc{visualStudioVersion}' in presetName:
                    return presetName

            elif platform.system() == "Darwin":
                isArm = platform.machine() == "arm64"
                isPresetForArm = "arm64" in presetName

                if "mac" in presetName:
                    xcodeFilter = (isXcodeInstalled and 'xcode' in presetName) or (
                        not isXcodeInstalled and 'xcode' not in presetName)
                    archFilter = (isArm and isPresetForArm) or (
                        not isArm and not isPresetForArm)
                    if xcodeFilter and archFilter:
                        return presetName

            elif platform.system() == "Linux":
                is_aarch = platform.machine() == "aarch64"
                is_preset_for_aarc = "aarch64" in presetName
                if "linux" in presetName:
                    if (is_aarch and is_preset_for_aarc) or (not is_aarch and not is_preset_for_aarc):
                        return presetName
        return None

    @classmethod
    def __GetValidBuiltLibraryDirectory(cls):
        if os.name == 'nt':
            requiredLibraryNames = [
                'PhysXCommon_64.dll',
                'PhysXCommon_64.dll',
                'PhysXCooking_64.dll',
                'PhysXFoundation_64.dll',
                'PhysXExtensions_64.dll'
            ]
        else:
            requiredLibraryNames = [
                'libPhysX_static_64.a',
                'libPhysXCommon_static_64.a',
                'libPhysXCooking_static_64.a',
                'libPhysXFoundation_static_64.a',
                'libPhysXExtensions_static_64.a'
            ]

        binDirectory = cls.__PhysXRootDirectory / 'physx/bin'
        if not binDirectory.exists():
            return None

        innerBinDirs = [x for x in os.listdir(
            binDirectory) if (binDirectory / x).is_dir()]

        for dir in innerBinDirs:  # example dir = macos.x86_64
            success = True
            for library in requiredLibraryNames:
                if not os.path.isfile(binDirectory / dir / 'checked' / library) or not os.path.isfile(binDirectory / dir / 'release' / library):
                    success = success and False
            if success:
                return binDirectory / dir
        return None


if __name__ == '__main__':
    ColoredLogger.InitLogger()
    if not PhysXSDKRequirements.Validate():
        PhysXSDKRequirements.BuildPhysXSDK()
