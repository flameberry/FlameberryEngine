#pragma once

#include "ECS/Scene.h"
#include "OpenGL/OpenGLBuffer.h"

namespace Flameberry {
    class SceneRenderer
    {
    public:
        SceneRenderer();
        ~SceneRenderer();

        void RenderScene(const std::shared_ptr<Scene>& scene, const PerspectiveCamera& camera);

        static std::shared_ptr<SceneRenderer> Create() { return std::make_shared<SceneRenderer>(); }
    private:
        std::shared_ptr<OpenGLShader> m_MeshShader;
        OpenGLBuffer m_SceneUniformBuffer;
    };
}