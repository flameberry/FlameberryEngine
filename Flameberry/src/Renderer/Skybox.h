#pragma once

#include <stdint.h>
#include "PerspectiveCamera.h"

namespace Flameberry {
    class Skybox
    {
    public:
        Skybox();
        Skybox(const char* folderPath);
        ~Skybox();

        void OnDraw(const PerspectiveCamera& camera);
        void Load(const char* folderPath);
    private:
        uint32_t m_VertexArrayID, m_VertexBufferID, m_IndexBufferID, m_ShaderProgramID, m_TextureID;
    };
}
