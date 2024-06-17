#pragma once

#include <vulkan/vulkan.h>

#include "Asset/Asset.h"
#include "Renderer/Image.h"
#include "Renderer/DescriptorSet.h"

namespace Flameberry {

    class Skymap : public Asset
    {
    public:
        Skymap(const std::filesystem::path& filepath);
        ~Skymap();

        Ref<DescriptorSet> GetDescriptorSet() { return m_SkymapDescriptorSet; }

        static void Init();
        static void Destroy();
        static Ref<DescriptorSet> GetEmptyDescriptorSet() { return s_EmptyDescriptorSet; }

        FBY_DECLARE_ASSET_TYPE(AssetType::Skymap);
    private:
        Ref<Image> m_CubemapImage, m_IrradianceMap, m_PrefilteredMap, m_BRDFLUTMap;

        Ref<DescriptorSet> m_SkymapDescriptorSet;

        // The sampler that allows sampling multiple LODs, which is very important for this pipeline
        VkSampler m_MultiLODSampler;
        // The sampler that is essential for BDRFLUT pipeline, to ensure no weird artefacts
        VkSampler m_BRDFLUTSampler;

        static Ref<DescriptorSet> s_EmptyDescriptorSet;
        static Ref<Image> s_EmptyCubemap;

        static std::unordered_map<uint32_t, Ref<Image>> s_CubemapSizeToBRDFLUTMap;
    };

}
