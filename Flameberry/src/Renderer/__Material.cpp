#include "__Material.h"

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
    }

    __Material::~__Material()
    {
        if (m_PushConstantBuffer)
            free(m_PushConstantBuffer);
    }

}
