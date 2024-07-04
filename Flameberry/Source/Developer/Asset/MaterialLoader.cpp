#include "MaterialLoader.h"

namespace Flameberry {

    Ref<Asset> MaterialLoader::LoadMaterial(const std::filesystem::path& path)
    {
        auto materialAsset = MaterialAssetSerializer::Deserialize(path.c_str());

        // Set Asset Class Variables
        materialAsset->FilePath = path;
        materialAsset->SizeInBytesOnCPU = materialAsset->GetUnderlyingMaterial()->GetUniformDataSize();
        // TODO: Fix this
//        materialAsset->SizeInBytesOnGPU = materialAsset->AlbedoMap->SizeInBytesOnGPU
//            + materialAsset->NormalMap->SizeInBytesOnGPU
//            + materialAsset->m_RoughnessMap->SizeInBytesOnGPU
//            + materialAsset->m_MetallicMap->SizeInBytesOnGPU
//            + materialAsset->m_AmbientOcclusionMap->SizeInBytesOnGPU;

        return materialAsset;
    }

}
