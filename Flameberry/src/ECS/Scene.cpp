// #include "Scene.h"
// #include "Core/Timer.h"
// #include "Component.h"

// namespace Flameberry {
//     Scene::Scene(ecs::registry* registry)
//         : m_Registry(registry)
//     {}

//     void Scene::RenderScene(OpenGLRenderer2D* renderer, const OrthographicCamera& camera)
//     {
//         renderer->Begin(camera);
//         for (const auto& entity : m_Registry->view<TransformComponent, SpriteRendererComponent>())
//         {
//             const auto& [transform, sprite] = m_Registry->get<TransformComponent, SpriteRendererComponent>(entity);
//             if (sprite.TextureFilePath == "")
//                 renderer->AddQuad(transform.GetTransform(), sprite.Color, (uint32_t)entity);
//             else
//                 renderer->AddQuad(transform.GetTransform(), sprite.TextureFilePath.c_str(), (uint32_t)entity);
//         }
//         renderer->End();
//     }
// }
