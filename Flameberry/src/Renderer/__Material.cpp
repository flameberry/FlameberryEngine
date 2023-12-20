#include "__Material.h"

#include "VulkanContext.h"

namespace Flameberry {

    __Material::__Material(const char* name, const Ref<Shader>& shader)
        : m_Name(name), m_Shader(shader)
    {
        // Arranging resources for the push constants
        // Currently the push constant blocks which are material specific (i.e. not Renderer Only) are accumulated and the combined data is stored together
        // As of now I haven't encountered a case where multiple material specific push constant blocks are used
        for (const auto& specification : m_Shader->GetPushConstantSpecifications())
        {
            // This is to remind the developer to implement this case properly when such a shader is written
            FBY_ASSERT(!m_PushConstantBufferSize, "Multiple push constants not handled correctly yet!");

            if (!specification.RendererOnly)
                m_PushConstantBufferSize += specification.Size;
        }

        if (m_PushConstantBufferSize)
            m_PushConstantBuffer = (uint8_t*)malloc(m_PushConstantBufferSize);

        // Arranging resources for the other Uniforms like Images and Uniform Buffers
        const auto& reflectionDescriptorSets = m_Shader->GetDescriptorSetSpecifications();
        const auto& descriptorBindings = m_Shader->GetDescriptorBindingSpecifications();

        std::vector<VkDescriptorSetLayoutBinding> vulkanDescSetBindings;
        uint32_t index = 0;

        for (const auto& reflectionDescSet : reflectionDescriptorSets)
        {
            for (uint32_t i = 0; i < reflectionDescSet.BindingCount; i++)
            {
                if (descriptorBindings[index].RendererOnly)
                {
                    index++;
                    continue;
                }

                vulkanDescSetBindings.emplace_back(VkDescriptorSetLayoutBinding{
                    .binding = descriptorBindings[index].Binding,
                    .descriptorCount = descriptorBindings[index].Count,
                    .descriptorType = descriptorBindings[index].Type,
                    .stageFlags = descriptorBindings[index].VulkanShaderStage,
                    .pImmutableSamplers = nullptr
                    }
                );

                index++;
            }

            // Create the descriptor set if the descriptor bindings are not RendererOnly
            if (vulkanDescSetBindings.size())
            {
                DescriptorSetSpecification descSetSpecification;
                descSetSpecification.Pool = VulkanContext::GetCurrentGlobalDescriptorPool();

                DescriptorSetLayoutSpecification layoutSpecification{ vulkanDescSetBindings };
                descSetSpecification.Layout = CreateRef<DescriptorSetLayout>(layoutSpecification);

                auto descSet = CreateRef<DescriptorSet>(descSetSpecification);
                m_DescriptorSets.push_back(descSet);

                vulkanDescSetBindings.clear();
            }
        }
    }

    __Material::~__Material()
    {
        if (m_PushConstantBuffer)
            free(m_PushConstantBuffer);
    }

}
