#pragma once

#include <vulkan/vulkan.h>

// TODO: Should this include be in the cpp file only with forward declaration and heap allocation of spv_reflect::ShaderModule?
#include <SPIRV-Reflect/spirv_reflect.h>

namespace Flameberry {

    struct PushConstantSpecification
    {
        const char* Name;
        VkShaderStageFlagBits ShaderStage;
        uint32_t Size, Offset;
        bool RendererOnly = true;
    };

    struct UniformVariableSpecification
    {
        const char* Name;
        uint32_t LocalOffset, GlobalOffset, Size;
    };

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
        Shader(const char* vertexShaderSpvPath, const char* fragmentShaderSpvPath);
        Shader(const char* spvBinaryPath);
        ~Shader();

        void GetVertexAndFragmentShaderModules(VkShaderModule* outVertexShaderModule, VkShaderModule* outFragmentShaderModule) const;

        VkShaderModule GetVulkanShaderModule() const;
        const std::vector<ShaderDataType>& GetInputVariableDataTypes() const { return m_InputDataTypes; }
        const std::vector<PushConstantSpecification>& GetPushConstantSpecifications() const { return m_PushConstantSpecifications; }

        // Costly functions
        const UniformVariableSpecification& Get(const std::string& name) const;
    private:
        std::vector<char> LoadShaderSpvCode(const char* path);
        void Reflect(const std::vector<char>& shaderSpvBinaryCode);
        VkShaderModule CreateVulkanShaderModule(const std::vector<char>& shaderSpvBinaryCode);
    private:
        std::string m_Name;

        union
        {
            VkShaderModule m_ShaderModule;
            struct
            {
                VkShaderModule m_VertexShaderModule;
                VkShaderModule m_FragmentShaderModule;
            };
        };

        VkShaderStageFlags m_VulkanShaderStageFlags = 0;

        std::vector<ShaderDataType> m_InputDataTypes;
        std::vector<PushConstantSpecification> m_PushConstantSpecifications;

        // This should only be accessed while initalisation of material or while editing it.
        // Don't access this during rendering every frame. As std::string hashing causes overhead
        // Note: The full names of the uniforms are used to access it, for e.g.: `u_Material.Roughness` instead of just `Roughness`
        // If no alias is given to the push constant block, then only the variable name is considered as full name, e.g.: `u_Roughness`
        std::unordered_map<std::string, UniformVariableSpecification> m_UniformFullNameToSpecification;
    };

}
