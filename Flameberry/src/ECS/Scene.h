#pragma once

#include "Registry.h"
#include "Renderer/OpenGL/OpenGLRenderer2D.h"

namespace Flameberry {
    class Scene
    {
    public:
        Scene(Registry* registry);
        ~Scene() = default;

        Registry* GetRegistry() { return m_Registry; }
        void RenderScene(OpenGLRenderer2D* renderer, const OrthographicCamera& camera);
    private:
        Registry* m_Registry;
    };
}