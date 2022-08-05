import os
import platform
from utils import fl_project_dir, build_fl_engine

if __name__ == '__main__':
    if platform.system() != 'Darwin' and platform.system() != 'Windows':
        print(
            "[FLAMEBERRY]: Flameberry Engine currently only supports Windows and MacOS!")
    else:
        os.chdir(fl_project_dir)
        build_fl_engine()
