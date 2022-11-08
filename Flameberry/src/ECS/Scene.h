#pragma once

#include "Registry.h"
#include "Renderer/OpenGL/OpenGLRenderer2D.h"
#include "Renderer/OpenGL/OpenGLRenderer3D.h"

namespace Flameberry {
    class Scene
    {
    public:
        Scene(Registry* registry);
        ~Scene() = default;

        Registry* GetRegistry() { return m_Registry; }
        void RenderScene(OpenGLRenderer2D* renderer, const OrthographicCamera& camera);
        void RenderScene(OpenGLRenderer3D* renderer, const PerspectiveCamera& camera);
        void SetSelectedEntity(entity_handle* entity) { m_SelectedEntity = entity; }
    private:
        entity_handle* m_SelectedEntity = nullptr;
        Registry* m_Registry;
    };
}