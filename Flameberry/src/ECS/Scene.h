#pragma once

#include "Registry.h"
#include "Renderer/Renderer2D.h"

namespace Flameberry {
    class Scene
    {
    public:
        Scene(Registry* registry);
        ~Scene() = default;

        void RenderScene(Renderer2D* renderer, OrthographicCamera& camera);
    private:
        Registry* m_Registry;
    };
}