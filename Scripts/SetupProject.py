import os, platform, subprocess
from Logger import ColoredLogger
from Utils import Utils

class ProjectRequirements:
    @classmethod
    def Validate(cls):
        try:
            if platform.system() == 'Windows':
                if cls.__IsVisualStudioInstalled():
                    subprocess.run([os.path.abspath(f"{Utils.GetProjectDirectory()}/Scripts/Win-GenProjects.bat"), "nopause"])
                else:
                    subprocess.run([os.path.abspath(f"{Utils.GetProjectDirectory()}/Scripts/Win-AutoGenAndBuild.bat"), "nopause"])
            
            elif platform.system() == 'Darwin':
                if cls.__IsXcodeInstalled():
                    subprocess.run(["sh", os.path.abspath(f"{Utils.GetProjectDirectory()}/Scripts/Unix-GenProjects.sh")])
                else:
                    subprocess.run(["sh", os.path.abspath(f"{Utils.GetProjectDirectory()}/Scripts/Unix-AutoGenAndBuild.sh")])
            
            else:
                subprocess.run(["sh", os.path.abspath(f"{Utils.GetProjectDirectory()}/Scripts/Unix-AutoGenAndBuild.sh")])
        
        except Exception as e:
                    ColoredLogger.Logger.error(e)

    @classmethod
    def __IsVisualStudioInstalled(cls):
        try:
            # Run the reg command to query the Windows Registry for Visual Studio installation
            result = subprocess.run(['reg', 'query', 'HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio'], capture_output=True, text=True)

            # Check if the command was successful (return code 0) and if Visual Studio keys are present in the output
            return result.returncode == 0 and 'VisualStudio' in result.stdout

        except Exception as e:
            ColoredLogger.Logger.error(e)
            return False
    
    @classmethod
    def __IsXcodeInstalled(cls):
        try:
            # Run the xcode-select command to check if Xcode is installed
            result = subprocess.run(['xcode-select', '--print-path'], capture_output=True, text=True)

            # Check if the command was successful (return code 0) and if Xcode path is present in the output
            return result.returncode == 0 and '/Applications/Xcode.app' in result.stdout.strip()

        except Exception as e:
            ColoredLogger.Logger.error(e)
            return False

if __name__ == '__main__':
    ProjectRequirements.Validate()