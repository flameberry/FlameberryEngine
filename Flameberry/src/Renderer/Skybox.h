#pragma once

#include <stdint.h>
#include "PerspectiveCamera.h"
#include "OpenGL/OpenGLShader.h"

namespace Flameberry {
    class Skybox
    {
    public:
        Skybox();
        Skybox(const char* folderPath);
        ~Skybox();

        void OnDraw(const PerspectiveCamera& camera);
        void Load(const char* folderPath);
        uint32_t GetTextureID() const { return m_TextureID; }
    private:
        uint32_t m_VertexArrayID, m_VertexBufferID, m_IndexBufferID, m_ShaderProgramID, m_TextureID;
        std::shared_ptr<OpenGLShader> m_SkyboxShader;
    };
}
