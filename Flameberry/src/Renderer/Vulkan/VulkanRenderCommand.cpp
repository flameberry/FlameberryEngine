#include "VulkanRenderCommand.h"

#include <fstream>

#include "Core/Core.h"

namespace Flameberry {
    std::vector<char> VulkanRenderCommand::LoadCompiledShaderCode(const std::string& filePath)
    {
        std::ifstream stream(filePath, std::ios::ate | std::ios::binary);
        FL_ASSERT(stream.is_open(), "Failed to open the file '{0}'", filePath);

        size_t fileSize = (size_t)stream.tellg();

        FL_INFO("File size of buffer taken from '{0}' is {1}", filePath, fileSize);

        std::vector<char> buffer(fileSize);

        stream.seekg(0);
        stream.read(buffer.data(), fileSize);

        stream.close();
        return buffer;
    }
}