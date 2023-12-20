#include "Shader.h"

#include <fstream>
#include <SPIRV-Reflect/spirv_reflect.h>

#include "Core/Core.h"
#include "Renderer/VulkanContext.h"
#include "Renderer/VulkanDebug.h"

#define FBY_SHADER_RENDERER_ONLY_PREFIX "_FBY_"

namespace Flameberry {

    namespace Utils {

        bool HasPrefix(const char* str1, const char* prefix)
        {
            while (*str1 != '\0' && *prefix != '\0')
            {
                if (*str1 != *prefix)
                    return false;
                ++str1;
                ++prefix;
            }
            return true;
        }

    }

    static ShaderDataType ReflectionFormatToShaderInputDataType(SpvReflectFormat format)
    {
        switch (format)
        {
            case SPV_REFLECT_FORMAT_R32_UINT:            return ShaderDataType::UInt;
            case SPV_REFLECT_FORMAT_R32_SINT:            return ShaderDataType::Int;
            case SPV_REFLECT_FORMAT_R32_SFLOAT:          return ShaderDataType::Float;
            case SPV_REFLECT_FORMAT_R32G32_SFLOAT:       return ShaderDataType::Float2;
            case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:    return ShaderDataType::Float3;
            case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return ShaderDataType::Float4;
            case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT: return ShaderDataType::Float4;
            default:
                FBY_ASSERT(0, "Type Description of Flag: {} not handled by Shader Class!");
                return ShaderDataType::None;
        }
    }

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

        spv_reflect::ShaderModule reflectionShaderModule(spv_data.size(), spv_data.data());
        if (reflectionShaderModule.GetResult() != SPV_REFLECT_RESULT_SUCCESS)
            FBY_ERROR("Could not process: {} (Is it a valid SPIR-V bytecode?)", spvBinaryPath);

#if 0
        if (reflectionShaderModule.GetShaderStage() == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
        {
            uint32_t count = 0;
            auto result = reflectionShaderModule.EnumerateInputVariables(&count, NULL);
            FBY_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to Enumerate SPIRV-Reflect Input Variables for shader: {}", spvBinaryPath);
            std::vector<SpvReflectInterfaceVariable*> vars(count);
            result = reflectionShaderModule.EnumerateInputVariables(&count, vars.data());
            FBY_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to Enumerate SPIRV-Reflect Input Variables for shader: {}", spvBinaryPath);

            m_InputDataTypes.resize(count);

            // Iterating through the reflection input variables to store the exact data type all variables belong to
            for (uint32_t i = 0; i < count; i++)
            {
                // This ASSERT is purely for ensuring that the reflection input variables are stored by sorting their locations
                FBY_ASSERT(vars[i]->location == i, "It seems like EnumerateInputVariables() didn't provide variables sorted by their locations!");

                // Don't include the shader input data type if it is a built-in GLSL Variable
                if (vars[i]->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
                    continue;

                // Check if the GLSL Variable has one of the Generic Data Types and then go deeper
                // and find out the exact data type using the `ReflectionFormatToShaderInputDataType` function
                if (vars[i]->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL)
                    m_InputDataTypes[i] = ShaderDataType::Bool;
                if (vars[i]->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_INT)
                    m_InputDataTypes[i] = ReflectionFormatToShaderInputDataType(vars[i]->format);
                if (vars[i]->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
                    m_InputDataTypes[i] = ReflectionFormatToShaderInputDataType(vars[i]->format);
                if (vars[i]->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
                {
                    // Currently only square matrices of 3 or 4 size are considered
                    // Because till now it has never been needed to even use a matrix as GLSL input variable
                    if (vars[i]->numeric.matrix.row_count == vars[i]->numeric.matrix.column_count)
                    {
                        switch (vars[i]->numeric.matrix.row_count)
                        {
                            case 3: m_InputDataTypes[i] = ShaderDataType::Float3x3;
                            case 4: m_InputDataTypes[i] = ShaderDataType::Float4x4;
                            default: FBY_ASSERT(0, "Only Mat3 and Mat4 are handled as input variable types!"); break;
                        }
                    }
                }

                // This assert is to ensure that no input data type has been missed or not recognised
                // So when the ASSERT is hit the above code can be rewritten to ensure that the particular case is handled
                FBY_ASSERT(m_InputDataTypes[i] != ShaderDataType::None, "The Shader Input Data Type is not recognized!");
                FBY_LOG("(location = {}) in {}: {} bits;", vars[i]->location, vars[i]->name, vars[i]->numeric.scalar.width);
            }
        }
#endif

        // The information about push constants is collected here
        uint32_t count = 0;
        auto result = reflectionShaderModule.EnumeratePushConstantBlocks(&count, NULL);
        FBY_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to Enumerate SPIRV-Reflect Push Constant Blocks for shader: {}", spvBinaryPath);
        std::vector<SpvReflectBlockVariable*> pcblocks(count);
        result = reflectionShaderModule.EnumeratePushConstantBlocks(&count, pcblocks.data());
        FBY_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to Enumerate SPIRV-Reflect Push Constant Blocks for shader: {}", spvBinaryPath);

        m_PushConstantSpecifications.resize(count);
        for (uint32_t i = 0; i < count; i++)
        {
            uint32_t absoluteSize = 0;
            for (uint32_t j = 0; j < pcblocks[i]->member_count; j++)
            {
                auto& member = pcblocks[i]->members[j];
                absoluteSize += member.size;
                FBY_LOG("Member Name: {}", member.name);

                std::string fullName(pcblocks[i]->name);
                if (!fullName.empty())
                    fullName += ".";
                fullName += member.name;

                // Full name is used to ensure uniqueness of the uniform variable (even though it is rare to overlap)
                m_UniformFullNameToSpecification[fullName] = UniformVariableSpecification{
                    .Name = member.name,
                    .LocalOffset = member.offset - pcblocks[i]->offset,
                    .GlobalOffset = member.offset,
                    .Size = member.size
                };
            }

            m_PushConstantSpecifications[i].Name = pcblocks[i]->type_description->type_name;
            m_PushConstantSpecifications[i].Offset = pcblocks[i]->offset;
            m_PushConstantSpecifications[i].Size = absoluteSize;
            m_PushConstantSpecifications[i].ShaderStage = (VkShaderStageFlagBits)reflectionShaderModule.GetShaderStage();
            m_PushConstantSpecifications[i].RendererOnly = Utils::HasPrefix(pcblocks[i]->type_description->type_name, FBY_SHADER_RENDERER_ONLY_PREFIX);
        }

        // Create the Vulkan Shader Module
        VkShaderModuleCreateInfo shaderModuleCreateInfo{};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = spv_data.size();
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(spv_data.data());

        const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &m_ShaderModule));
    }

    const UniformVariableSpecification& Shader::Get(const std::string& name) const
    {
        auto it = m_UniformFullNameToSpecification.find(name);
        FBY_ASSERT(it != m_UniformFullNameToSpecification.end(), "Uniform does not exist in shader: {}", name);
        return it->second;
    }

    Shader::~Shader()
    {
        const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyShaderModule(device, m_ShaderModule, nullptr);
    }

}
