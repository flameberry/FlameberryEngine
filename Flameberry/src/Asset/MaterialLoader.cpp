#include "MaterialLoader.h"

namespace Flameberry {

    std::shared_ptr<Asset> MaterialLoader::LoadMaterial(const std::filesystem::path& path)
    {
        auto materialAsset = MaterialSerializer::Deserialize(path.c_str());

        // Set Asset Class Variables
        materialAsset->FilePath = path;
        materialAsset->SizeInBytesOnCPU = sizeof(Material);
        materialAsset->SizeInBytesOnGPU = materialAsset->TextureMap->SizeInBytesOnGPU
            + materialAsset->NormalMap->SizeInBytesOnGPU
            + materialAsset->RoughnessMap->SizeInBytesOnGPU
            + materialAsset->MetallicMap->SizeInBytesOnGPU
            + materialAsset->AmbientOcclusionMap->SizeInBytesOnGPU;

        return materialAsset;
    }

}