from Logger import ColoredLogger
from SetupCMake import CMakeRequirements
from SetupVulkan import VulkanSDKRequirements
from SetupPhysX import PhysXSDKRequirements
from SetupProject import ProjectRequirements

if __name__ == "__main__":
    ColoredLogger.InitLogger()
    ColoredLogger.Logger.info("Initialized logger!")

    if not CMakeRequirements.Validate():
        CMakeRequirements.InstallCMake()

    CMakeRequirements.WriteChosenDirectoryToMeta()

    if not VulkanSDKRequirements.Validate():
        VulkanSDKRequirements.InstallVulkanSDK()

    ProjectRequirements.Validate()

    ColoredLogger.Logger.info("Finished Setup!")
