#pragma once

#include "Core/Core.h"
#include "Shader.h"
#include "DescriptorSet.h"

namespace Flameberry {

    // This is the experimental class which is still being developed
    class __Material
    {
    public:
        __Material(const char* name, const Ref<Shader>& shader);
        ~__Material();

        template<typename T>
        void Set(const std::string& uniformName, const T& value)
        {
            const auto& uniformVar = m_Shader->Get(uniformName);
            FBY_ASSERT(sizeof(T) == uniformVar.Size, "Size of uniform variable {} ({}) does not match the size of the value ({}) given!", uniformName, uniformVar.Size, sizeof(T));
            FBY_ASSERT(uniformVar.LocalOffset + uniformVar.Size <= m_PushConstantBufferSize, "Uniform variable '{}' has invalid location as offset + size > {}", uniformName, m_PushConstantBufferSize);

            memcpy(m_PushConstantBuffer + uniformVar.LocalOffset, &value, uniformVar.Size);
        }

        void Set(const std::string& uniformName, const void* data, std::size_t size)
        {
            const auto& uniformVar = m_Shader->Get(uniformName);
            FBY_ASSERT(size == uniformVar.Size, "Size of uniform variable {} ({}) does not match the size of the value ({}) given!", uniformName, uniformVar.Size, size);
            FBY_ASSERT(uniformVar.LocalOffset + uniformVar.Size <= m_PushConstantBufferSize, "Uniform variable '{}' has invalid location as offset + size > {}", uniformName, m_PushConstantBufferSize);

            memcpy(m_PushConstantBuffer + uniformVar.LocalOffset, data, uniformVar.Size);
        }

#if 1
        // This is strictly a developer/debug only function to test if the data is stored appropriately in the m_PushConstantBuffer
        template<typename T>
        T Get(const std::string& uniformName)
        {
            const auto& uniformVar = m_Shader->Get(uniformName);
            FBY_ASSERT(sizeof(T) == uniformVar.Size, "Size of uniform variable {} does not match the size of the type given!", uniformName);
            FBY_ASSERT(uniformVar.LocalOffset + uniformVar.Size <= m_PushConstantBufferSize, "Uniform variable '{}' has invalid location as offset + size > {}", uniformName, m_PushConstantBufferSize);

            return (*reinterpret_cast<T*>(&m_PushConstantBuffer[uniformVar.LocalOffset]));
        }

        // This is strictly a developer/debug only function to test if the data is stored appropriately in the m_PushConstantBuffer
        template<typename T>
        const T* GetArray(const std::string& uniformName)
        {
            const auto& uniformVar = m_Shader->Get(uniformName);
            FBY_ASSERT(uniformVar.Size % sizeof(T) == 0, "Size of uniform array {} is not a multiple of the size of the type given!", uniformName);
            FBY_ASSERT(uniformVar.LocalOffset + uniformVar.Size <= m_PushConstantBufferSize, "Uniform variable '{}' has invalid location as offset + size > {}", uniformName, m_PushConstantBufferSize);

            return reinterpret_cast<const T*>(&m_PushConstantBuffer[uniformVar.LocalOffset]);
        }
#endif
    private:
        // Wondering if this should be std::string
        const char* m_Name;

        // The core element behind the material is this shader
        Ref<Shader> m_Shader;

        uint32_t m_PushConstantBufferSize = 0;
        // This is the main push constant data that will be sent to the shader directly
        uint8_t* m_PushConstantBuffer = nullptr;

        // Descriptor Sets required
        std::vector<Ref<DescriptorSet>> m_DescriptorSets;
    };

}
