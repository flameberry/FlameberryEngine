#pragma once

#include <vulkan/vulkan.h>

// TODO: Should this include be in the cpp file only with forward declaration and heap allocation of spv_reflect::ShaderModule?
#include <SPIRV-Reflect/spirv_reflect.h>

namespace Flameberry {

    class Shader
    {
    public:
        Shader(const char* spvBinaryPath);
        ~Shader();

        VkShaderModule GetVulkanShaderModule() const { return m_ShaderModule; }
    private:
        spv_reflect::ShaderModule m_ReflectionShaderModule;
        VkShaderModule m_ShaderModule;
    };

}
