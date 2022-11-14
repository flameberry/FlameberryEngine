#include "OpenGLRenderer3D.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "OpenGLRenderCommand.h"
#include "Core/Core.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

#include "Renderer/ModelLoader.h"
#include "Core/Timer.h"

namespace Flameberry {
    void OpenGLRenderer3D::UpdateViewportSize()
    {
        int width, height;
        glfwGetFramebufferSize(m_UserGLFWwindow, &width, &height);
        m_ViewportSize = { (float)width, (float)height };
    }

    void OpenGLRenderer3D::Begin(const PerspectiveCamera& camera)
    {
        UpdateViewportSize();
        m_AspectRatio = m_ViewportSize.x / m_ViewportSize.y;

        m_UniformBufferData.ViewProjectionMatrix = camera.GetViewProjectionMatrix();

        /* Set Projection Matrix in GPU memory, for all shader programs to access it */
        glBindBuffer(GL_UNIFORM_BUFFER, m_UniformBufferId);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(m_UniformBufferData.ViewProjectionMatrix));
    }

    void OpenGLRenderer3D::End()
    {
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void OpenGLRenderer3D::OnDraw()
    {
        static float rotation = 0.0f;
        static double prevTime = glfwGetTime();

        double crntTime = glfwGetTime();
        if (crntTime - prevTime >= 1 / 60)
        {
            rotation += 0.5f;
            prevTime = crntTime;
        }

        glm::mat4 model0 = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        glm::mat4 model1 = glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));

        m_TempMesh.Draw(model0, m_PointLights);
        m_SponzaMesh.Draw(model1, m_PointLights);
    }

    void OpenGLRenderer3D::Init(GLFWwindow* window)
    {
#ifdef WIN32
        auto [vertices_1, indices_1] = OpenGLRenderCommand::LoadModel(FL_PROJECT_DIR"SandboxApp/assets/models/sphere.obj");
        m_TempMesh = { vertices_1, indices_1 };
        auto [v, i] = OpenGLRenderCommand::LoadModel(FL_PROJECT_DIR"SandboxApp/assets/models/sponza.obj");
        m_SponzaMesh = Mesh{ v, i };
#else
        auto [vertices, alt_indices] = ModelLoader::LoadOBJ(FL_PROJECT_DIR"SandboxApp/assets/models/sphere.obj");
        m_TempMesh = Mesh{ vertices, alt_indices };
        auto [v, i] = ModelLoader::LoadOBJ(FL_PROJECT_DIR"SandboxApp/assets/models/sponza.obj");
        m_SponzaMesh = Mesh{ v, i };
#endif
        m_TextureId = OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/brick.png");

        m_TempMesh.TextureIDs.push_back(OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/brick.png"));
        m_SponzaMesh.TextureIDs.push_back(m_TextureId);

        // m_PointLights.push_back(PointLight(glm::vec3(1.0f), glm::vec4(1.0f), 5.0f));

        // Set point light data
        // m_PointLights.emplace_back();
        // m_PointLights.back().Position = glm::vec3(0.4, 1, 1);
        // m_PointLights.back().Color = glm::vec4(0, 0, 1, 1);
        // m_PointLights.back().Intensity = 2.0f;

        // m_PointLights.emplace_back();
        // m_PointLights.back().Position = glm::vec3(0, 0, 1.5);
        // m_PointLights.back().Color = glm::vec4(1, 1, 0, 1);
        // m_PointLights.back().Intensity = 2.0f;

        // m_PointLights.emplace_back();
        // m_PointLights.back().Position = glm::vec3(0, 1, 1);
        // m_PointLights.back().Color = glm::vec4(1, 0.5, 0, 1);
        // m_PointLights.back().Intensity = 2.0f;

        m_PointLights.emplace_back();
        m_PointLights.back().Position = glm::vec3(0, 10, 1);
        m_PointLights.back().Color = glm::vec4(1, 0, 1, 1);
        m_PointLights.back().Intensity = 30000.0f;

        m_PointLights.emplace_back();
        m_PointLights.back().Position = glm::vec3(10, 7.5, 1);
        m_PointLights.back().Color = glm::vec4(1, 0.5, 0, 1);
        m_PointLights.back().Intensity = 30000.0f;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);

        /* Create Uniform Buffer */
        glGenBuffers(1, &m_UniformBufferId);
        glBindBuffer(GL_UNIFORM_BUFFER, m_UniformBufferId);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBufferData), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferRange(GL_UNIFORM_BUFFER, 0, m_UniformBufferId, 0, sizeof(UniformBufferData));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        m_UserGLFWwindow = window;
        UpdateViewportSize();
    }

    void OpenGLRenderer3D::CleanUp()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }

    std::shared_ptr<OpenGLRenderer3D> OpenGLRenderer3D::Create()
    {
        return std::make_shared<OpenGLRenderer3D>();
    }
}
