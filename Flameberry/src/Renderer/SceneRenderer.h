#pragma once

#include "ECS/Scene.h"
#include "OpenGL/OpenGLBuffer.h"
#include "Renderer/OpenGL/OpenGLRenderer2D.h"
#include <memory>

namespace Flameberry {
    class SceneRenderer
    {
    public:
        SceneRenderer();
        ~SceneRenderer();

        void RenderScene(const std::shared_ptr<Scene>& scene, const PerspectiveCamera& camera, const glm::mat4& lightViewProjectionMatrix);
        void RenderSceneForShadowMapping(const std::shared_ptr<Scene>& scene, const std::shared_ptr<OpenGLShader>& shader);
        void RenderSceneForMousePicking(const std::shared_ptr<Scene>& scene, const std::shared_ptr<OpenGLShader>& shader);

        void RenderLights(const std::shared_ptr<Scene>& scene, const std::shared_ptr<OpenGLRenderer2D>& renderer2D, const glm::mat4& cameraViewMatrix);
        void RenderLightsForMousePicking(const std::shared_ptr<Scene>& scene, const std::shared_ptr<OpenGLRenderer2D>& renderer2D, const glm::mat4& cameraViewMatrix, const std::shared_ptr<OpenGLShader>& mousePickingShader);

        static std::shared_ptr<SceneRenderer> Create() { return std::make_shared<SceneRenderer>(); }
        void ReloadShader();
    private:
    private:
        std::shared_ptr<OpenGLShader> m_MeshShader;
        OpenGLBuffer m_SceneUniformBuffer, m_MeshCameraUniformBuffer;
    };
}