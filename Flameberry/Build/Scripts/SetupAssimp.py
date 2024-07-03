import os, platform, subprocess
from Logger import ColoredLogger
from Utils import Utils

class AssimpRequirements:
    __AssimpRootDirectory = Utils.GetProjectDirectory() / 'Flameberry/Source/ThirdParty/Assimp'

    @classmethod
    def Validate(cls) -> bool:
        pf = platform.system()
        libraryName = 'Assimp.dll' if pf == 'Windows' else 'libassimp.dylib' if pf == 'Darwin' else 'libassimp.so' if pf == 'Linux' else ''
        binDirectory = cls.__AssimpRootDirectory / 'build/bin'

        if binDirectory.exists():
            for item in os.listdir(binDirectory):
                if (binDirectory / item).is_file() and item == libraryName:
                    ColoredLogger.Logger.info(f'Found Assimp at: {binDirectory / item}')
                    return True
        
        ColoredLogger.Logger.warning(f'Failed to find Assimp Library (Must not be built)!')
        return False
    
    @classmethod
    def BuildAssimp(cls):
        try:
            buildDir = cls.__AssimpRootDirectory / "build"
            subprocess.run(['cmake', f'-S{cls.__AssimpRootDirectory}', f'-B{buildDir}'])
            subprocess.run(['cmake', '--build', f'{buildDir}'])
        except Exception as e:
            raise e

if __name__ == '__main__':
    ColoredLogger.InitLogger()
    if not AssimpRequirements.Validate():
        AssimpRequirements.BuildAssimp()