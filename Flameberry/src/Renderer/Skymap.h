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
        Ref<Image> m_CubemapImage, m_IrradianceMap, m_PrefilteredMap;

        Ref<DescriptorSet> m_SkymapDescriptorSet;

        static Ref<DescriptorSet> s_EmptyDescriptorSet;
        static Ref<Image> s_EmptyCubemap;
    };

}
