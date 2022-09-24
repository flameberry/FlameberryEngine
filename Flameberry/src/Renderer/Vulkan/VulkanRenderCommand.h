#pragma once

#include <vector>

namespace Flameberry {
    class VulkanRenderCommand
    {
    public:
        static std::vector<char> LoadCompiledShaderCode(const std::string& filePath);
    private:
    };
}