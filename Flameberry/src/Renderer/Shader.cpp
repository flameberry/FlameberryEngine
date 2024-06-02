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

    }

    Shader::Shader(const char* vertexShaderSpvPath, const char* fragmentShaderSpvPath)
    {
        // TODO: This is probably some odd behaviour to name the shader with just the vertex shader's name
        const auto& stem = std::filesystem::path(vertexShaderSpvPath).stem().string();
        m_Name = stem.substr(0, stem.find('.'));

        // Vertex Shader
        std::vector<char> vertexShaderSpvBinaryCode = LoadShaderSpvCode(vertexShaderSpvPath);
        Reflect(vertexShaderSpvBinaryCode);
        m_VertexShaderModule = CreateVulkanShaderModule(vertexShaderSpvBinaryCode);

        // Fragment Shader
        std::vector<char> fragmentShaderSpvBinaryCode = LoadShaderSpvCode(fragmentShaderSpvPath);
        Reflect(fragmentShaderSpvBinaryCode);
        m_FragmentShaderModule = CreateVulkanShaderModule(fragmentShaderSpvBinaryCode);
    }

    Shader::Shader(const char* spvBinaryPath)
    {
        const auto& stem = std::filesystem::path(spvBinaryPath).stem().string();
        m_Name = stem.substr(0, stem.find('.'));

        std::vector<char> shaderSpvBinaryCode = LoadShaderSpvCode(spvBinaryPath);
        Reflect(shaderSpvBinaryCode);
        m_ShaderModule = CreateVulkanShaderModule(shaderSpvBinaryCode);
    }

    const ReflectionUniformVariableSpecification& Shader::GetUniform(const std::string& name) const
    {
        auto it = m_UniformFullNameToSpecification.find(name);
        FBY_ASSERT(it != m_UniformFullNameToSpecification.end(), "Uniform does not exist in shader: {}", name);
        return it->second;
    }

    const ReflectionDescriptorBindingSpecification& Shader::GetBinding(const std::string& name) const
    {
        auto it = m_DescriptorBindingVariableFullNameToSpecificationIndex.find(name);
        FBY_ASSERT(it != m_DescriptorBindingVariableFullNameToSpecificationIndex.end(), "Descriptor Binding does not exist in shader: {}", name);
        return m_DescriptorBindingSpecifications[it->second];
    }

    std::vector<char> Shader::LoadShaderSpvCode(const char* path)
    {
        std::ifstream spv_ifstream(path, std::ios::binary);
        if (!spv_ifstream.is_open())
            FBY_ERROR("Failed to open SPIRV binary: {}", path);

        spv_ifstream.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(spv_ifstream.tellg());
        spv_ifstream.seekg(0, std::ios::beg);

        std::vector<char> shaderSpvBinaryCode(size);
        spv_ifstream.read(shaderSpvBinaryCode.data(), size);
        return std::move(shaderSpvBinaryCode);
    }

    void Shader::Reflect(const std::vector<char>& shaderSpvBinaryCode)
    {
        spv_reflect::ShaderModule reflectionShaderModule(shaderSpvBinaryCode.size(), shaderSpvBinaryCode.data());
        if (reflectionShaderModule.GetResult() != SPV_REFLECT_RESULT_SUCCESS)
            FBY_ERROR("Could not process: {} (Is it a valid SPIR-V bytecode?)", m_Name);

        // Store the shader stage flags
        m_VulkanShaderStageFlags |= reflectionShaderModule.GetShaderStage();

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
                    m_InputDataTypes[i] = Utils::ReflectionFormatToShaderInputDataType(vars[i]->format);
                if (vars[i]->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
                    m_InputDataTypes[i] = Utils::ReflectionFormatToShaderInputDataType(vars[i]->format);
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

        {
            // The information about push constants is collected here
            uint32_t count = 0;
            auto result = reflectionShaderModule.EnumeratePushConstantBlocks(&count, NULL);
            FBY_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to Enumerate SPIRV-Reflect Push Constant Blocks for shader: {}", m_Name);
            std::vector<SpvReflectBlockVariable*> pcblocks(count);
            result = reflectionShaderModule.EnumeratePushConstantBlocks(&count, pcblocks.data());
            FBY_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to Enumerate SPIRV-Reflect Push Constant Blocks for shader: {}", m_Name);

            m_PushConstantSpecifications.reserve(m_PushConstantSpecifications.size() + count);
            for (uint32_t i = 0; i < count; i++)
            {
                uint32_t absoluteSize = 0;
                FBY_LOG("Shader `{}` - PushConstantBlock:", m_Name, i);

                for (uint32_t j = 0; j < pcblocks[i]->member_count; j++)
                {
                    auto& member = pcblocks[i]->members[j];
                    absoluteSize += member.size;
                    FBY_LOG("\tMember Name: {}", member.name);

                    std::string fullName(pcblocks[i]->name);
                    if (!fullName.empty())
                        fullName += ".";
                    fullName += member.name;

                    // Full name is used to ensure uniqueness of the uniform variable (even though it is rare to overlap)
                    m_UniformFullNameToSpecification[fullName] = ReflectionUniformVariableSpecification{
                        .Name = fullName.c_str(), // Wondering if the full name should be used
                        .LocalOffset = member.offset - pcblocks[i]->offset,
                        .GlobalOffset = member.offset,
                        .Size = member.size
                    };
                }

                m_PushConstantSpecifications.emplace_back(ReflectionPushConstantSpecification{
                    .Name = pcblocks[i]->type_description->type_name,
                    .Offset = pcblocks[i]->offset,
                    .Size = absoluteSize,
                    .VulkanShaderStage = (VkShaderStageFlagBits)reflectionShaderModule.GetShaderStage(),
                    .RendererOnly = Utils::HasPrefix(pcblocks[i]->type_description->type_name, FBY_SHADER_RENDERER_ONLY_PREFIX)
                    }
                );

            }
        }

        {
            // The information about descriptor sets is collected here
            uint32_t count = 0;
            auto result = reflectionShaderModule.EnumerateDescriptorSets(&count, NULL);
            FBY_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to Enumerate SPIRV-Reflect Push Constant Blocks for shader: {}", m_Name);
            std::vector<SpvReflectDescriptorSet*> descSets(count);
            result = reflectionShaderModule.EnumerateDescriptorSets(&count, descSets.data());
            FBY_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to Enumerate SPIRV-Reflect Push Constant Blocks for shader: {}", m_Name);

            m_DescriptorSetSpecifications.reserve(m_DescriptorSetSpecifications.size() + count);
            // Iterating through the reflection descriptor sets to store the binding information needed to create a descriptor set
            for (uint32_t i = 0; i < count; i++)
            {
                m_DescriptorBindingSpecifications.reserve(m_DescriptorBindingSpecifications.size() + descSets[i]->binding_count);
                for (uint32_t j = 0; j < descSets[i]->binding_count; j++)
                {
                    auto& binding = descSets[i]->bindings[j];

                    std::string fullName = binding->name;
                    bool rendererOnly, isDescriptorTypeImage;

                    if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER
                        || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                        || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE
                        || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE
                        || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
                    {
                        isDescriptorTypeImage = true;
                        // An image descriptor will be considered renderer only when it's name has the following prefix
                        rendererOnly = Utils::HasPrefix(binding->name, FBY_SHADER_RENDERER_ONLY_PREFIX);
                    }
                    else
                    {
                        isDescriptorTypeImage = false;
                        // A buffer descriptor will be considered renderer only when it's TYPE name (not the alias name) has the following prefix
                        rendererOnly = Utils::HasPrefix(binding->type_description->type_name, FBY_SHADER_RENDERER_ONLY_PREFIX);
                        // Is this the right way to deal with this? Being able to refer an Uniform Buffer with it's alias
                        // If alias is not present then with the Uniform Type name
                        if (fullName.empty())
                            fullName = binding->type_description->type_name;
                    }

                    // Here all the reflection binding specifications are pushed back
                    // Assumption here is that all the bindings are in order (Yet to find out how would it affect the functioning)
                    m_DescriptorBindingSpecifications.emplace_back(ReflectionDescriptorBindingSpecification{
                        .Name = fullName,
                        .Set = binding->set,
                        .Binding = binding->binding,
                        .Count = binding->count,
                        .Type = (VkDescriptorType)binding->descriptor_type,
                        .VulkanShaderStage = (VkShaderStageFlags)reflectionShaderModule.GetShaderStage(),
                        .RendererOnly = rendererOnly,
                        .IsDescriptorTypeImage = isDescriptorTypeImage
                        }
                    );

                    // This unordered_map is stored only for convenience of setting the Uniform Buffers/Images using their names in the shader
                    // It shouldn't be accessed every frame
                    m_DescriptorBindingVariableFullNameToSpecificationIndex[fullName] = m_DescriptorBindingSpecifications.size() - 1;
                }
                m_DescriptorSetSpecifications.emplace_back(ReflectionDescriptorSetSpecification{ .Set = descSets[i]->set, .BindingCount = descSets[i]->binding_count });
            }
        }
    }

    VkShaderModule Shader::CreateVulkanShaderModule(const std::vector<char>& shaderSpvBinaryCode)
    {
        VkShaderModuleCreateInfo shaderModuleCreateInfo{};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = shaderSpvBinaryCode.size();
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderSpvBinaryCode.data());

        const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VkShaderModule vulkanShaderModule = VK_NULL_HANDLE;
        VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &vulkanShaderModule));
        return vulkanShaderModule;
    }

    Shader::~Shader()
    {
        const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        if (m_VulkanShaderStageFlags & (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT))
        {
            vkDestroyShaderModule(device, m_VertexShaderModule, nullptr);
            vkDestroyShaderModule(device, m_FragmentShaderModule, nullptr);
        }
        else
        {
            vkDestroyShaderModule(device, m_ShaderModule, nullptr);
        }
    }

    void Shader::GetVertexAndFragmentShaderModules(VkShaderModule* outVertexShaderModule, VkShaderModule* outFragmentShaderModule) const
    {
        FBY_ASSERT(m_VulkanShaderStageFlags & (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), "GetVertexAndFragmentShaderModules() should only be called for Shader made using Vertex and Fragment components both!");
        *outVertexShaderModule = m_VertexShaderModule;
        *outFragmentShaderModule = m_FragmentShaderModule;
    }

    VkShaderModule Shader::GetVulkanShaderModule() const
    {
        FBY_ASSERT((m_VulkanShaderStageFlags > 0) && ((m_VulkanShaderStageFlags & (m_VulkanShaderStageFlags - 1)) == 0), "GetVulkanShaderModule() should only be called for Shader made using single component like Compute or Vertex or Fragment but not a combination of them!");
        return m_ShaderModule;
    }

}
