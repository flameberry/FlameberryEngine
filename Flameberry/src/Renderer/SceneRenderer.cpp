#include "SceneRenderer.h"

namespace Flameberry {
    struct MeshCameraUniformBufferData {
        glm::mat4 ViewProjectionMatrix;
        glm::mat4 LightViewProjectionMatrix;
    };

    struct SceneUniformBufferData {
        alignas(16) glm::vec3 CameraPosition;
        DirectionalLight DirLight;
        PointLight PointLights[10];
        int LightCount = 0;
    };

    SceneRenderer::SceneRenderer()
        : m_MeshCameraUniformBuffer(sizeof(MeshCameraUniformBufferData), nullptr, GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW),
        m_SceneUniformBuffer(sizeof(SceneUniformBufferData), nullptr, GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW)
    {
        m_MeshCameraUniformBuffer.BindBufferBase(FL_UNIFORM_BLOCK_BINDING_CAMERA);
        m_SceneUniformBuffer.BindBufferBase(FL_UNIFORM_BLOCK_BINDING_LIGHTING);

        OpenGLShaderBinding binding;
        binding.blockBindingIndex = FL_UNIFORM_BLOCK_BINDING_CAMERA;
        binding.blockName = "Camera";

        OpenGLShaderBinding lightBinding;
        binding.blockBindingIndex = FL_UNIFORM_BLOCK_BINDING_LIGHTING;
        binding.blockName = "Lighting";

        m_MeshShader = OpenGLShader::Create(FL_PROJECT_DIR"Flameberry/assets/shaders/Default.glsl", { binding, lightBinding });
        m_MeshShader->Bind();
        m_MeshShader->PushUniformInt("u_TextureMapSampler", 0);
        m_MeshShader->PushUniformInt("u_ShadowMapSampler", 1);
        m_MeshShader->Unbind();
    }

    SceneRenderer::~SceneRenderer() {}

    void SceneRenderer::RenderScene(const std::shared_ptr<Scene>& scene, const PerspectiveCamera& camera, const glm::mat4& lightViewProjectionMatrix)
    {
        MeshCameraUniformBufferData meshCameraUniformBufferData;
        meshCameraUniformBufferData.ViewProjectionMatrix = camera.GetViewProjectionMatrix();
        meshCameraUniformBufferData.LightViewProjectionMatrix = lightViewProjectionMatrix;

        m_MeshCameraUniformBuffer.Bind();
        m_MeshCameraUniformBuffer.BufferSubData(&meshCameraUniformBufferData, sizeof(MeshCameraUniformBufferData), 0);

        SceneUniformBufferData sceneUniformBufferData;
        sceneUniformBufferData.CameraPosition = camera.GetPosition();
        sceneUniformBufferData.DirLight = scene->m_SceneData.DirLight;
        for (const auto& entity : scene->m_Registry->view<TransformComponent, LightComponent>())
        {
            const auto& [transform, light] = scene->m_Registry->get<TransformComponent, LightComponent>(entity);
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Position = transform.translation;
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Color = light.Color;
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Intensity = light.Intensity;
            sceneUniformBufferData.LightCount++;
        }

        m_SceneUniformBuffer.Bind();
        m_SceneUniformBuffer.BufferSubData(&sceneUniformBufferData, sizeof(SceneUniformBufferData), 0);

        scene->m_SceneData.ActiveSkybox.OnDraw(camera);
        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

            if (scene->m_SceneData.Materials.find(mesh.MaterialName) != scene->m_SceneData.Materials.end())
                scene->m_SceneData.Meshes[mesh.MeshIndex].Draw(m_MeshShader, transform, scene->m_SceneData.Materials[mesh.MaterialName], (uint32_t)entity);
            else
                scene->m_SceneData.Meshes[mesh.MeshIndex].Draw(m_MeshShader, transform, Material(), (uint32_t)entity);
        }

        m_SceneUniformBuffer.Unbind();
        m_MeshCameraUniformBuffer.Unbind();
    }

    void SceneRenderer::RenderSceneForShadowMapping(const std::shared_ptr<Scene>& scene, const std::shared_ptr<OpenGLShader>& shader)
    {
        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);
            scene->m_SceneData.Meshes[mesh.MeshIndex].DrawForShadowPass(shader, transform);
        }
    }

    void SceneRenderer::ReloadShader()
    {
        OpenGLShaderBinding binding;
        binding.blockBindingIndex = FL_UNIFORM_BLOCK_BINDING_CAMERA;
        binding.blockName = "Camera";

        OpenGLShaderBinding lightBinding;
        binding.blockBindingIndex = FL_UNIFORM_BLOCK_BINDING_LIGHTING;
        binding.blockName = "Lighting";

        m_MeshShader.reset(new OpenGLShader(FL_PROJECT_DIR"Flameberry/assets/shaders/Default.glsl", { binding, lightBinding }));
        m_MeshShader->Bind();
        m_MeshShader->PushUniformInt("u_TextureMapSampler", 0);
        m_MeshShader->PushUniformInt("u_ShadowMapSampler", 1);
        m_MeshShader->Unbind();

        FL_LOG("Reloaded Mesh Shader!");
    }
}
