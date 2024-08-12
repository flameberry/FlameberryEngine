import os, platform, subprocess, pathlib
from Logger import ColoredLogger
from Utils import Utils


class VulkanSDKRequirements:
    __VulkanMinimumVersionRequired = "1.3."
    __VulkanVersionPreferred = "1.3.268.0"
    __MacOSVulkanSDKInstallPathGlobal = os.path.expanduser(f"~/VulkanSDK/{__VulkanVersionPreferred}")
    __VulkanSDKInstallPathLocal = (
        Utils.GetProjectDirectory() / f"Flameberry/Intermediate/ThirdParty/VulkanSDK/{__VulkanVersionPreferred}"
    )
    __ShouldInstallVulkanSDKLocally = True

    @classmethod
    def Validate(cls) -> bool:
        vulkanSDK = os.environ.get("VULKAN_SDK")
        if vulkanSDK is None:
            ColoredLogger.Logger.error("Vulkan SDK is not installed (VULKAN_SDK env-variable is None)")
            return False

        if cls.__VulkanMinimumVersionRequired not in vulkanSDK:
            ColoredLogger.Logger.error(
                f"Vulkan SDK version installed not supported by Flameberry Engine: Required Version: {cls.__VulkanMinimumVersionRequired}x.x"
            )
            return False

        ColoredLogger.Logger.info(f"Found Vulkan SDK at {vulkanSDK}")
        return True

    @classmethod
    def InstallVulkanSDK(cls):
        URL, extension = cls.__ConstructVulkanSDKURL()
        filename = f"VulkanSDKInstaller{extension}"
        installerPath = Utils.GetProjectDirectory() / "Flameberry/Intermediate/ThirdParty/VulkanSDK" / filename
        Utils.DownloadFile(URL, installerPath)

        # For Windows installation only
        if URL.endswith(".exe"):
            try:
                if cls.__ShouldInstallVulkanSDKLocally:
                    installParams = [
                        installerPath,
                        "--root",
                        cls.__VulkanSDKInstallPathLocal,
                        "--accept-licenses",
                        "--default-answer",
                        "--confirm-command",
                        "install",
                    ]
                else:
                    installParams = [
                        installerPath,
                        "--accept-licenses",
                        "--default-answer",
                        "--confirm-command",
                        "install",
                    ]
                subprocess.run(installParams)
            except Exception as e:
                raise e

        # For macOS installation only
        elif URL.endswith(".dmg"):
            # Attach DMG Volume
            subprocess.run(["hdiutil", "attach", installerPath])
            volumePath = f"/Volumes/vulkansdk-macos-{cls.__VulkanVersionPreferred}"
            vulkanInstallerPath = f"/Volumes/vulkansdk-macos-{cls.__VulkanVersionPreferred}/InstallVulkan.app/Contents/MacOS/InstallVulkan"

            vulkanSDKPath = str(
                cls.__VulkanSDKInstallPathLocal
                if cls.__ShouldInstallVulkanSDKLocally
                else cls.__MacOSVulkanSDKInstallPathGlobal
            )
            # Install the Vulkan SDK
            try:
                installParams = [
                    vulkanInstallerPath,
                    "--root",
                    vulkanSDKPath,
                    "--accept-licenses",
                    "--default-answer",
                    "--confirm-command",
                    "install",
                ]
                subprocess.run(installParams)
            except Exception as e:
                subprocess.run(["hdiutil", "detach", volumePath])
                raise e

            # Detach DMG Volume
            subprocess.run(["hdiutil", "detach", volumePath])

            # Add VulkanSDK to PATH
            homeDirectory = os.path.expanduser("~")
            shellProfilePath = os.path.join(
                homeDirectory,
                (
                    ".zshrc"
                    if os.path.exists(os.path.expanduser("~/.zshrc"))
                    else ".zprofile" if os.path.exists(os.path.expanduser("~/.zprofile")) else ".bash_profile"
                ),
            )
            with open(shellProfilePath, "a") as profile:
                profile.write("\n# Vulkan SDK Setup (Installed by Flameberry)\n")
                profile.write(f"source {vulkanSDKPath}/setup-env.sh\n")

            # TODO: Maybe obsolete
            os.environ["VULKAN_SDK"] = f'{os.pathsep}{vulkanSDKPath + "/macOS"}'

        # For Linux installation only
        else:
            ColoredLogger.Logger.warning(
                f"Vulkan SDK Tar file is downloaded at: {installerPath}. You have to further install the Vulkan SDK referring to this documentation: https://vulkan.lunarg.com/doc/sdk/latest/linux/getting_started.html"
            )
            ColoredLogger.Logger.warning(
                f"Make sure to add VULKAN_SDK to Environment PATH before running the script again."
            )
            quit()

        os.remove(installerPath)

    @classmethod
    def __ConstructVulkanSDKURL(cls) -> tuple[str, str]:
        URLS = {
            "Windows": (
                f"https://sdk.lunarg.com/sdk/download/{cls.__VulkanVersionPreferred}/windows/VulkanSDK-{cls.__VulkanVersionPreferred}-Installer.exe",
                ".exe",
            ),
            "Darwin": (
                f"https://sdk.lunarg.com/sdk/download/{cls.__VulkanVersionPreferred}/mac/vulkansdk-macos-{cls.__VulkanVersionPreferred}.dmg",
                ".dmg",
            ),
            "Linux": (
                f"https://sdk.lunarg.com/sdk/download/{cls.__VulkanVersionPreferred}/linux/vulkansdk-linux-x86_64-{cls.__VulkanVersionPreferred}.tar.xz",
                ".tar.xz",
            ),
        }
        return URLS[platform.system()]


if __name__ == "__main__":
    ColoredLogger.InitLogger()
    # if not VulkanSDKRequirements.Validate():
    VulkanSDKRequirements.InstallVulkanSDK()
