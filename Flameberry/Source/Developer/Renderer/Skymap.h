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

        FBY_DECLARE_ASSET_TYPE(AssetType::Skymap);
    private:
        Ref<Image> m_CubemapImage;

        Ref<DescriptorSet> m_SkymapDescriptorSet;
    };

}
