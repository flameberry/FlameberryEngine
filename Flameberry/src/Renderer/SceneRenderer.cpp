#include "SceneRenderer.h"

#include <glm/gtc/type_ptr.hpp>

#include "ECS/Component.h"
#include "OpenGL/OpenGLUniformBufferIndices.h"


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
        bool EnvironmentMapReflections = false;
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
        binding.blockName = "SceneData";

        m_MeshShader = OpenGLShader::Create(FL_PROJECT_DIR"Flameberry/assets/shaders/opengl/mesh.vert", FL_PROJECT_DIR"Flameberry/assets/shaders/opengl/mesh.frag", { binding, lightBinding });
        m_MeshShader->Bind();
        m_MeshShader->PushUniformInt("u_TextureMapSampler", 0);
        m_MeshShader->PushUniformInt("u_ShadowMapSampler", 1);
        m_MeshShader->PushUniformInt("u_EnvironmentMapSampler", 2);
        m_MeshShader->Unbind();

        m_BulbTexture = OpenGLTexture::Create(FL_PROJECT_DIR"FlameEditor/icons/bulb_icon_v4.png", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
    }

    SceneRenderer::~SceneRenderer() {}

    void SceneRenderer::RenderScene(
        const std::shared_ptr<Scene>& scene,
        const PerspectiveCamera& camera,
        const glm::mat4& lightViewProjectionMatrix,
        const std::shared_ptr<OpenGLRenderer2D>& renderer2D
    )
    {
        MeshCameraUniformBufferData meshCameraUniformBufferData;
        meshCameraUniformBufferData.ViewProjectionMatrix = camera.GetViewProjectionMatrix();
        meshCameraUniformBufferData.LightViewProjectionMatrix = lightViewProjectionMatrix;

        m_MeshCameraUniformBuffer.Bind();
        m_MeshCameraUniformBuffer.BufferSubData(&meshCameraUniformBufferData, sizeof(MeshCameraUniformBufferData), 0);

        SceneUniformBufferData sceneUniformBufferData;
        sceneUniformBufferData.CameraPosition = camera.GetPosition();
        sceneUniformBufferData.DirLight = scene->m_SceneData.ActiveEnvironmentMap.DirLight;
        for (const auto& entity : scene->m_Registry->view<TransformComponent, LightComponent>())
        {
            const auto& [transform, light] = scene->m_Registry->get<TransformComponent, LightComponent>(entity);
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Position = transform.translation;
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Color = light.Color;
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Intensity = light.Intensity;
            sceneUniformBufferData.LightCount++;
        }
        sceneUniformBufferData.EnvironmentMapReflections = scene->m_SceneData.ActiveEnvironmentMap.Reflections;

        m_SceneUniformBuffer.Bind();
        m_SceneUniformBuffer.BufferSubData(&sceneUniformBufferData, sizeof(SceneUniformBufferData), 0);

        if (scene->m_SceneData.ActiveEnvironmentMap.EnableSkybox)
        {
            scene->m_SceneData.ActiveEnvironmentMap.ActiveSkybox->OnDraw(camera);
            uint32_t skyboxTextureID = scene->m_SceneData.ActiveEnvironmentMap.ActiveSkybox->GetTextureID();

            glActiveTexture(GL_TEXTURE0 + 2);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureID);
        }

        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

            if (scene->m_SceneData.Materials.find(mesh.MaterialName) != scene->m_SceneData.Materials.end())
                scene->m_SceneData.Meshes[mesh.MeshIndex]->Draw(m_MeshShader, transform, scene->m_SceneData.Materials[mesh.MaterialName]);
            else
                scene->m_SceneData.Meshes[mesh.MeshIndex]->Draw(m_MeshShader, transform, Material());
        }

        m_SceneUniformBuffer.Unbind();

        // Render Lights in scene
        for (const auto& entity : scene->m_Registry->view<TransformComponent, LightComponent>())
        {
            const auto& [transform, light] = scene->m_Registry->get<TransformComponent, LightComponent>(entity);
            renderer2D->AddBillBoard(
                transform.translation,
                0.4f,
                m_BulbTexture,
                camera.GetViewMatrix()
            );
        }
        renderer2D->End();
    }

    void SceneRenderer::RenderSceneForShadowMapping(const std::shared_ptr<Scene>& scene, const std::shared_ptr<OpenGLShader>& shader)
    {
        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);
            scene->m_SceneData.Meshes[mesh.MeshIndex]->DrawForShadowPass(shader, transform);
        }
    }

    void SceneRenderer::RenderSceneForMousePicking(
        const std::shared_ptr<Scene>& scene,
        const PerspectiveCamera& camera,
        const std::shared_ptr<OpenGLShader>& shader,
        const std::shared_ptr<OpenGLRenderer2D>& renderer2D
    )
    {
        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);
            scene->m_SceneData.Meshes[mesh.MeshIndex]->DrawForMousePicking(shader, transform, (uint32_t)entity);
        }

        glm::mat4 identityMatrix(1.0f);
        shader->Bind();
        shader->PushUniformMatrix4f("u_ModelMatrix", 1, false, glm::value_ptr(identityMatrix));

        for (const auto& entity : scene->m_Registry->view<TransformComponent, LightComponent>())
        {
            const auto& [transform, light] = scene->m_Registry->get<TransformComponent, LightComponent>(entity);
            renderer2D->AddBillBoardForMousePicking(
                transform.translation,
                0.4f,
                camera.GetViewMatrix(),
                (uint32_t)entity
            );
        }
        renderer2D->FlushBatch(shader);
    }

    void SceneRenderer::ReloadShader()
    {
        OpenGLShaderBinding binding;
        binding.blockBindingIndex = FL_UNIFORM_BLOCK_BINDING_CAMERA;
        binding.blockName = "Camera";

        OpenGLShaderBinding lightBinding;
        binding.blockBindingIndex = FL_UNIFORM_BLOCK_BINDING_LIGHTING;
        binding.blockName = "SceneData";

        m_MeshShader.reset(new OpenGLShader(FL_PROJECT_DIR"Flameberry/assets/shaders/opengl/mesh.vert", FL_PROJECT_DIR"Flameberry/assets/shaders/opengl/mesh.frag", { binding, lightBinding }));
        m_MeshShader->Bind();
        m_MeshShader->PushUniformInt("u_TextureMapSampler", 0);
        m_MeshShader->PushUniformInt("u_ShadowMapSampler", 1);
        m_MeshShader->PushUniformInt("u_EnvironmentMapSampler", 2);
        m_MeshShader->Unbind();

        FL_LOG("Reloaded Mesh Shader!");
    }
}
