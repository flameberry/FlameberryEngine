import os, platform, subprocess
from Logger import ColoredLogger
from Utils import Utils

class ProjectRequirements:
    @classmethod
    def Validate(cls):
        try:
            if platform.system() == 'Windows':
                if Utils.GetVisualStudioVersionIfInstalled() is None: # if Version is None
                    subprocess.run([os.path.abspath(f"{Utils.GetProjectDirectory()}/Scripts/Win-GenProjects.bat"), "nopause"])
                else:
                    subprocess.run([os.path.abspath(f"{Utils.GetProjectDirectory()}/Scripts/Win-AutoGenAndBuild.bat"), "nopause"])
            
            elif platform.system() == 'Darwin':
                if Utils.IsXcodeInstalled():
                    subprocess.run(["sh", os.path.abspath(f"{Utils.GetProjectDirectory()}/Scripts/Unix-GenProjects.sh")])
                else:
                    subprocess.run(["sh", os.path.abspath(f"{Utils.GetProjectDirectory()}/Scripts/Unix-AutoGenAndBuild.sh")])
            
            else:
                subprocess.run(["sh", os.path.abspath(f"{Utils.GetProjectDirectory()}/Scripts/Unix-AutoGenAndBuild.sh")])
        
        except Exception as e:
                    ColoredLogger.Logger.error(e)

if __name__ == '__main__':
    ProjectRequirements.Validate()