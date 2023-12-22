#pragma once

#include "Core/Core.h"
#include "VulkanContext.h"
#include "Shader.h"
#include "DescriptorSet.h"
#include "Buffer.h"
#include "Texture2D.h"

namespace Flameberry {

    class Material
    {
    public:
        Material(const Ref<Shader>& shader);
        ~Material();

        uint32_t GetUniformDataSize() const { return m_PushConstantBufferSize; }
        uint32_t GetPushConstantOffset() const { return m_PushConstantBufferOffset; }
        const uint8_t* GetUniformDataPtr() const { return m_PushConstantBuffer; }

        template<typename T>
        void Set(const std::string& uniformName, const T& value)
        {
            const auto& uniformVar = m_Shader->GetUniform(uniformName);
            FBY_ASSERT(sizeof(T) == uniformVar.Size, "Size of uniform variable {} ({}) does not match the size of the value ({}) given", uniformName, uniformVar.Size, sizeof(T));
            FBY_ASSERT(uniformVar.LocalOffset + uniformVar.Size <= m_PushConstantBufferSize, "Uniform variable '{}' has invalid location as offset + size > {}", uniformName, m_PushConstantBufferSize);

            memcpy(m_PushConstantBuffer + uniformVar.LocalOffset, &value, uniformVar.Size);
        }

        void Set(const std::string& uniformName, const void* data, std::size_t size)
        {
            const auto& uniformVar = m_Shader->GetUniform(uniformName);
            FBY_ASSERT(size == uniformVar.Size, "Size of uniform variable {} ({}) does not match the size of the value ({}) given", uniformName, uniformVar.Size, size);
            FBY_ASSERT(uniformVar.LocalOffset + uniformVar.Size <= m_PushConstantBufferSize, "Uniform variable '{}' has invalid location as offset + size > {}", uniformName, m_PushConstantBufferSize);

            memcpy(m_PushConstantBuffer + uniformVar.LocalOffset, data, uniformVar.Size);
        }

        template<>
        void Set<Ref<Texture2D>>(const std::string& uniformName, const Ref<Texture2D>& texture)
        {
            const auto& binding = m_Shader->GetBinding(uniformName);
            FBY_ASSERT(binding.IsDescriptorTypeImage, "The requested uniform binding: {} is not an Image Descriptor");

            // How to assemble this?
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageView = texture->GetImageView();
            imageInfo.sampler = texture->GetSampler();
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            m_DescriptorSets[binding.Set - m_StartSetIndex]->WriteImage(binding.Binding, imageInfo);
            m_DescriptorSets[binding.Set - m_StartSetIndex]->Update();
        }

        // This one looks ugly, maybe the arguments should be abstracted away somehow...
        void Set(const std::string& uniformName, const Ref<Image>& image, VkSampler sampler, VkImageLayout imageLayout)
        {
            const auto& binding = m_Shader->GetBinding(uniformName);
            FBY_ASSERT(binding.IsDescriptorTypeImage, "The requested uniform binding: {} is not an Image Descriptor");

            // How to assemble this?
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageView = image->GetImageView();
            imageInfo.sampler = sampler;
            imageInfo.imageLayout = imageLayout;

            m_DescriptorSets[binding.Set - m_StartSetIndex]->WriteImage(binding.Binding, imageInfo);
            m_DescriptorSets[binding.Set - m_StartSetIndex]->Update();
        }

        // This function is a bit dodgy and the behaviour is not tested yet, be cautious
        template<>
        void Set<Ref<Buffer>>(const std::string& uniformName, const Ref<Buffer>& uniformBuffer)
        {
            const auto& binding = m_Shader->GetBinding(uniformName);
            FBY_ASSERT(!binding.IsDescriptorTypeImage, "The requested uniform binding: {} is not a Buffer Descriptor");

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffer->GetBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = uniformBuffer->GetBufferSize();

            m_DescriptorSets[binding.Set - m_StartSetIndex]->WriteBuffer(binding.Binding, bufferInfo);
            m_DescriptorSets[binding.Set - m_StartSetIndex]->Update();

            FBY_WARN("Material::Set() for Buffers is not tested and could produce undefined behaviour!");
        }

#if 1
        // This is strictly a developer/debug only function to test if the data is stored appropriately in the m_PushConstantBuffer
        template<typename T>
        T Get(const std::string& uniformName)
        {
            const auto& uniformVar = m_Shader->GetUniform(uniformName);
            FBY_ASSERT(sizeof(T) == uniformVar.Size, "Size of uniform variable {} does not match the size of the type given!", uniformName);
            FBY_ASSERT(uniformVar.LocalOffset + uniformVar.Size <= m_PushConstantBufferSize, "Uniform variable '{}' has invalid location as offset + size > {}", uniformName, m_PushConstantBufferSize);

            return (*reinterpret_cast<T*>(&m_PushConstantBuffer[uniformVar.LocalOffset]));
        }

        // This is strictly a developer/debug only function to test if the data is stored appropriately in the m_PushConstantBuffer
        template<typename T>
        const T* GetArray(const std::string& uniformName)
        {
            const auto& uniformVar = m_Shader->GetUniform(uniformName);
            FBY_ASSERT(uniformVar.Size % sizeof(T) == 0, "Size of uniform array {} is not a multiple of the size of the type given!", uniformName);
            FBY_ASSERT(uniformVar.LocalOffset + uniformVar.Size <= m_PushConstantBufferSize, "Uniform variable '{}' has invalid location as offset + size > {}", uniformName, m_PushConstantBufferSize);

            return reinterpret_cast<const T*>(&m_PushConstantBuffer[uniformVar.LocalOffset]);
        }
#endif
    protected:

        // This function is to be used by MaterialAsset class to access the data easily every frame without having to refer uniforms by their names
        template<typename T>
        inline T& GetUniformDataReferenceAs() const { return (T&)*reinterpret_cast<T*>(m_PushConstantBuffer); }
    private:
        // The core element behind the material is this shader
        Ref<Shader> m_Shader;

        // This is the main push constant data that will be sent to the shader directly
        uint8_t* m_PushConstantBuffer = nullptr;
        uint32_t m_PushConstantBufferSize = 0, m_PushConstantBufferOffset = 0;

        // Descriptor Sets required
        std::vector<Ref<DescriptorSet>> m_DescriptorSets;
        // This indicates the starting set index of the first descriptor set stored in `m_DescriptorSets`
        // This is purely on the assumption that the Renderer Only Sets will have lower set numbers and
        // the rest will be Material accessible sets
        // Hence it is assumed that these sets will be in ascending order starting from `m_StartSetIndex`
        uint32_t m_StartSetIndex = -1;

    private:
        friend class Renderer;
        friend class MaterialAsset;
    };

}
