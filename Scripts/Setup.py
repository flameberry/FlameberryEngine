from Logger import ColoredLogger
from SetupCMake import CMakeRequirements
from SetupVulkan import VulkanSDKRequirements
from SetupPhysX import PhysXSDKRequirements
from SetupAssimp import AssimpRequirements

if __name__ == '__main__':
    ColoredLogger.InitLogger()
    ColoredLogger.Logger.info('Initialized logger!')

    if not CMakeRequirements.Validate():
        CMakeRequirements.InstallCMake()
    
    if not VulkanSDKRequirements.Validate():
        VulkanSDKRequirements.InstallVulkanSDK()

    if not PhysXSDKRequirements.Validate():
        PhysXSDKRequirements.BuildPhysXSDK()

    if not AssimpRequirements.Validate():
        AssimpRequirements.BuildAssimp()
    
    ColoredLogger.Logger.info('Finished Setup!')