from Logger import ColoredLogger
from SetupCMake import CMakeRequirements
from SetupVulkan import VulkanSDKRequirements
from SetupPhysX import PhysXSDKRequirements
from SetupAssimp import AssimpRequirements
from SetupProject import ProjectRequirements

if __name__ == "__main__":
    ColoredLogger.InitLogger()
    ColoredLogger.Logger.info("Initialized logger!")

    if not CMakeRequirements.Validate():
        CMakeRequirements.InstallCMake()

    CMakeRequirements.WriteChosenDirectoryToMeta()

    if not VulkanSDKRequirements.Validate():
        VulkanSDKRequirements.InstallVulkanSDK()

    if not AssimpRequirements.Validate():
        AssimpRequirements.BuildAssimp()

    ProjectRequirements.Validate()

    ColoredLogger.Logger.info("Finished Setup!")
