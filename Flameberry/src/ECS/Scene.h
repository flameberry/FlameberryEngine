#pragma once

#include "Registry.h"
#include "Renderer/Renderer2D.h"

namespace Flameberry {
    class Scene
    {
    public:
        Scene(Registry* registry);
        ~Scene() = default;

        Registry* GetRegistry() { return m_Registry; }
        void RenderScene(Renderer2D* renderer, const OrthographicCamera& camera);
    private:
        Registry* m_Registry;
    };
}