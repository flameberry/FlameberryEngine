#pragma once

#include <vector>
#include "VulkanVertex.h"

namespace Flameberry {
    class VulkanRenderCommand
    {
    public:
        static std::vector<char> LoadCompiledShaderCode(const std::string& filePath);
        static std::tuple<std::vector<VulkanVertex>, std::vector<uint32_t>> LoadModel(const std::string& filePath);
    };
}
