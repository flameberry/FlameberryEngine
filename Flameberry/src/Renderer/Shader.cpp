#include "Shader.h"

#include <fstream>
#include <SPIRV-Reflect/spirv_reflect.h>

#include "Core/Core.h"
#include "Renderer/VulkanContext.h"
#include "Renderer/VulkanDebug.h"

namespace Flameberry {

    Shader::Shader(const char* spvBinaryPath)
    {
        std::ifstream spv_ifstream(spvBinaryPath, std::ios::binary);
        if (!spv_ifstream.is_open())
            FBY_ERROR("Failed to open SPIRV binary: {}", spvBinaryPath);

        spv_ifstream.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(spv_ifstream.tellg());
        spv_ifstream.seekg(0, std::ios::beg);

        std::vector<char> spv_data(size);
        spv_ifstream.read(spv_data.data(), size);

        m_ReflectionShaderModule = spv_reflect::ShaderModule(spv_data.size(), spv_data.data());
        if (m_ReflectionShaderModule.GetResult() != SPV_REFLECT_RESULT_SUCCESS)
            FBY_ERROR("Could not process: {} (Is it a valid SPIR-V bytecode?)", spvBinaryPath);

        uint32_t count = 0;

        auto result = m_ReflectionShaderModule.EnumerateDescriptorSets(&count, NULL);
        FBY_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to Enumerate SPIRV-Reflect Descriptor Sets for shader: {}", spvBinaryPath);
        std::vector<SpvReflectDescriptorSet*> sets(count);
        result = m_ReflectionShaderModule.EnumerateDescriptorSets(&count, sets.data());
        FBY_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to Enumerate SPIRV-Reflect Descriptor Sets for shader: {}", spvBinaryPath);

        for (uint32_t i = 0; i < count; i++)
        {
            for (uint32_t j = 0; j < sets[i]->binding_count; j++)
            {
                if (sets[i]->bindings[j]->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    FBY_LOG("{}", sets[i]->bindings[j]->name);
            }
        }

        // Create the Vulkan Shader Module
        VkShaderModuleCreateInfo shaderModuleCreateInfo{};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = spv_data.size();
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(spv_data.data());

        const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &m_ShaderModule));
    }

    Shader::~Shader()
    {
        const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyShaderModule(device, m_ShaderModule, nullptr);
    }

}
