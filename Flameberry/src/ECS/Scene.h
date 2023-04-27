// #pragma once

// #include "ecs.hpp"
// #include "Renderer/OpenGL/OpenGLRenderer2D.h"

// namespace Flameberry {
//     class Scene
//     {
//     public:
//         Scene(ecs::registry* registry);
//         ~Scene() = default;

//         ecs::registry* GetRegistry() { return m_Registry; }
//         void RenderScene(OpenGLRenderer2D* renderer, const OrthographicCamera& camera);
//         void SetSelectedEntity(ecs::entity_handle* entity) { m_SelectedEntity = entity; }
//     private:
//         ecs::entity_handle* m_SelectedEntity = nullptr;
//         ecs::registry* m_Registry;
//     };
// }