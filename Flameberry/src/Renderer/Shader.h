#pragma once

#include <vulkan/vulkan.h>

// TODO: Should this include be in the cpp file only with forward declaration and heap allocation of spv_reflect::ShaderModule?
#include <SPIRV-Reflect/spirv_reflect.h>

namespace Flameberry {

    enum class ShaderDataType : uint8_t
    {
        None = 0,
        UInt, UInt2, UInt3, UInt4,
        Int, Int2, Int3, Int4,
        Float, Float2, Float3, Float4,
        Float3x3, Float4x4,
        Bool,

        // Ensure that these Dummy Data Types are always at the end of the enum...
        // ...To avoid unwanted effects during creating Vertex Attribute Descriptions in Pipeline
        Dummy1, Dummy4, Dummy8, Dummy12, Dummy16 // Note: The sizes are in bytes
    };

    class Shader
    {
    public:
        Shader(const char* spvBinaryPath);
        ~Shader();

        VkShaderModule GetVulkanShaderModule() const { return m_ShaderModule; }
        const std::vector<ShaderDataType>& GetInputVariableDataTypes() const { return m_InputDataTypes; }
    private:
        spv_reflect::ShaderModule m_ReflectionShaderModule;
        VkShaderModule m_ShaderModule;

        std::vector<ShaderDataType> m_InputDataTypes;
    };

}
