#include "SceneRenderer.h"

namespace Flameberry {
    struct SceneUniformBufferData {
        alignas(16) glm::vec3 CameraPosition;
        DirectionalLight DirLight;
        PointLight PointLights[10];
        int LightCount = 0;
    };

    SceneRenderer::SceneRenderer()
        : m_SceneUniformBuffer(sizeof(SceneUniformBufferData), nullptr, GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW)
    {
        m_SceneUniformBuffer.BindBufferBase(1);

        OpenGLShaderBinding binding;
        binding.blockBindingIndex = 0;
        binding.blockName = "Camera";

        OpenGLShaderBinding lightBinding;
        binding.blockBindingIndex = 1;
        binding.blockName = "Lighting";

        m_MeshShader = OpenGLShader::Create(FL_PROJECT_DIR"Flameberry/assets/shaders/Default.glsl", { binding, lightBinding });
    }

    SceneRenderer::~SceneRenderer() {}

    void SceneRenderer::RenderScene(const std::shared_ptr<Scene>& scene, const PerspectiveCamera& camera)
    {
        SceneUniformBufferData sceneUniformBufferData;
        sceneUniformBufferData.CameraPosition = camera.GetPosition();
        sceneUniformBufferData.DirLight = scene->m_SceneData.DirLight;
        // if (scene->m_Registry->DoesComponentPoolExist<LightComponent>())
        // {
        //     const void* lightComponentBuffer = scene->m_Registry->GetComponentPool<LightComponent>()->Get();
        //     sceneUniformBufferData.LightCount = glm::min(FL_MAX_POINT_LIGHTS, (int)scene->m_Registry->GetComponentPool<LightComponent>()->size());
        //     memcpy(sceneUniformBufferData.PointLights, lightComponentBuffer, sceneUniformBufferData.LightCount);
        // }
        for (const auto& entity : scene->m_Registry->View<TransformComponent, LightComponent>())
        {
            const auto& [transform, light] = scene->m_Registry->Get<TransformComponent, LightComponent>(entity);
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Position = transform->translation;
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Color = light->Color;
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Intensity = light->Intensity;
            sceneUniformBufferData.LightCount++;
        }

        m_SceneUniformBuffer.Bind();
        m_SceneUniformBuffer.BufferSubData(&sceneUniformBufferData, sizeof(SceneUniformBufferData), 0);

        scene->m_SceneData.ActiveSkybox.OnDraw(camera);
        for (const auto& entity : scene->m_Registry->View<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->Get<TransformComponent, MeshComponent>(entity);

            if (scene->m_SceneData.Materials.find(mesh->MaterialName) != scene->m_SceneData.Materials.end())
                scene->m_SceneData.Meshes[mesh->MeshIndex].Draw(m_MeshShader, *transform, scene->m_SceneData.Materials[mesh->MaterialName], entity.get());
            else
                scene->m_SceneData.Meshes[mesh->MeshIndex].Draw(m_MeshShader, *transform, Material(), entity.get());
        }

        m_SceneUniformBuffer.Unbind();
    }
}
