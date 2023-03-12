#pragma once

#include <stdint.h>
#include "PerspectiveCamera.h"
#include "OpenGL/OpenGLShader.h"
#include "OpenGL/OpenGLTexture.h"

namespace Flameberry {
    class Skybox
    {
    public:
        Skybox();
        Skybox(const char* folderPath);
        ~Skybox();

        void OnDraw(const PerspectiveCamera& camera);
        void Load(const char* folderPath);

        uint32_t GetTextureID() const { return m_SkyboxTexture->GetTextureID(); }
        std::string GetFolderPath() const { return m_FolderPath; }
    private:
        std::string m_FolderPath;
        uint32_t m_VertexArrayID, m_VertexBufferID, m_IndexBufferID, m_ShaderProgramID;
        std::shared_ptr<OpenGLShader> m_SkyboxShader;
        std::shared_ptr<OpenGLTexture> m_SkyboxTexture;
    };
}
