#include "Mesh.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "OpenGL/OpenGLRenderCommand.h"
#include "OpenGL/OpenGLRenderer3D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Core/Core.h"

namespace Flameberry {
    Mesh::Mesh()
        : m_VertexArrayID(0), m_VertexBufferID(0), m_IndexBufferID(0), m_ShaderProgramID(0)
    {
    }

    Mesh::Mesh(const std::vector<OpenGLVertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name)
        : Vertices(vertices), Indices(indices), Name(name), m_VertexArrayID(0), m_VertexBufferID(0), m_IndexBufferID(0), m_ShaderProgramID(0)
    {
        Invalidate();
    }

    void Mesh::BindShader()
    {
        glUseProgram(m_ShaderProgramID);
    }

    void Mesh::SetUniformLight(const PointLight& light, uint32_t index)
    {
        std::string uniformName = "u_PointLights[" + std::to_string(index) + "]";

        glUniform3f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Position"), light.Position.x, light.Position.y, light.Position.z);
        glUniform4f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Color"), light.Color.x, light.Color.y, light.Color.z, light.Color.w);
        glUniform1f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Intensity"), light.Intensity);
    }

    void Mesh::Invalidate()
    {
        if (Vertices.size() && Indices.size() && m_VertexArrayID && m_VertexBufferID && m_IndexBufferID && m_ShaderProgramID)
            return;

        glGenVertexArrays(1, &m_VertexArrayID);
        glBindVertexArray(m_VertexArrayID);

        glGenBuffers(1, &m_VertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, 1000000 * sizeof(OpenGLVertex), nullptr, GL_DYNAMIC_DRAW);

        glBindVertexArray(m_VertexArrayID);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 1 * sizeof(int) + 13 * sizeof(float), (void*)offsetof(OpenGLVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 1 * sizeof(int) + 13 * sizeof(float), (void*)offsetof(OpenGLVertex, color));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 1 * sizeof(int) + 13 * sizeof(float), (void*)offsetof(OpenGLVertex, normal));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 1 * sizeof(int) + 13 * sizeof(float), (void*)offsetof(OpenGLVertex, texture_uv));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(int) + 13 * sizeof(float), (void*)offsetof(OpenGLVertex, texture_index));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 1, GL_INT, GL_FALSE, 1 * sizeof(int) + 13 * sizeof(float), (void*)offsetof(OpenGLVertex, entityID));

        glGenBuffers(1, &m_IndexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, Indices.size() * sizeof(uint32_t), Indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(m_VertexArrayID);

        m_ShaderProgramID = OpenGLRenderCommand::CreateShader(FL_PROJECT_DIR"Flameberry/assets/shaders/Default.glsl");
        glUseProgram(m_ShaderProgramID);
        int samplers[16];
        for (uint32_t i = 0; i < 16; i++)
            samplers[i] = i;
        glUniform1iv(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_TextureSamplers"), 16, samplers);

        // Assigning binding to uniform buffers
        uint32_t uniformBlockIndexCamera = glGetUniformBlockIndex(m_ShaderProgramID, "Camera");
        glUniformBlockBinding(m_ShaderProgramID, uniformBlockIndexCamera, FL_UNIFORM_BLOCK_BINDING_CAMERA);

        // uint32_t uniformBlockIndexLighting = glGetUniformBlockIndex(m_ShaderProgramID, "Lighting");
        // glUniformBlockBinding(m_ShaderProgramID, uniformBlockIndexLighting, FL_UNIFORM_BLOCK_BINDING_LIGHTING);

        glUseProgram(0);
    }

    void Mesh::Draw(const glm::mat4& transform)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, Vertices.size() * sizeof(OpenGLVertex), Vertices.data());

        for (uint16_t i = 0; i < TextureIDs.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, TextureIDs[i]);
        }

        glUseProgram(m_ShaderProgramID);
        glUniformMatrix4fv(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_ModelMatrix"), 1, GL_FALSE, glm::value_ptr(transform));

        glBindVertexArray(m_VertexArrayID);
        glDrawElements(GL_TRIANGLES, (int)Indices.size(), GL_UNSIGNED_INT, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void Mesh::Draw(const glm::mat4& transform, const glm::vec3& cameraPosition, const std::vector<PointLight>& lights)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, Vertices.size() * sizeof(OpenGLVertex), Vertices.data());

        for (uint16_t i = 0; i < TextureIDs.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, TextureIDs[i]);
        }

        glUseProgram(m_ShaderProgramID);
        glUniformMatrix4fv(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_ModelMatrix"), 1, GL_FALSE, glm::value_ptr(transform));
        glUniform1i(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_LightCount"), (int)lights.size());

        glm::vec3 camPosition = glm::mat3(glm::inverse(transform)) * cameraPosition;
        // glm::vec3 camPosition = cameraPosition;
        glUniform3f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_CameraPosition"), camPosition.x, camPosition.y, camPosition.z);

        for (uint32_t i = 0; i < lights.size(); i++)
        {
            std::string uniformName = "u_PointLights[" + std::to_string(i) + "]";
            const PointLight& light = lights[i];
            // glm::vec3 lightPosition = glm::mat3(glm::transpose(glm::inverse(transform))) * light.Position;
            glm::vec3 lightPosition = glm::mat3(glm::inverse(transform)) * light.Position;
            // glm::vec3 lightPosition = light.Position;

            glUniform3f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Position"), lightPosition.x, lightPosition.y, lightPosition.z);
            glUniform4f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Color"), light.Color.x, light.Color.y, light.Color.z, light.Color.w);
            glUniform1f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Intensity"), light.Intensity);
        }

        glBindVertexArray(m_VertexArrayID);
        glDrawElements(GL_TRIANGLES, (int)Indices.size(), GL_UNSIGNED_INT, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void Mesh::Draw(const TransformComponent& transform, const glm::vec3& cameraPosition, const std::vector<PointLight>& lights, int entityID)
    {
        if (m_EntityID != entityID)
        {
            m_EntityID = entityID;
            for (auto& vertex : Vertices)
                vertex.entityID = m_EntityID;
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, Vertices.size() * sizeof(OpenGLVertex), Vertices.data());

        for (uint16_t i = 0; i < TextureIDs.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, TextureIDs[i]);
        }

        glUseProgram(m_ShaderProgramID);
        glUniformMatrix4fv(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_ModelMatrix"), 1, GL_FALSE, glm::value_ptr(transform.GetTransform()));
        glUniform1i(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_LightCount"), (int)lights.size());
        glUniform3f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_CameraPosition"), cameraPosition.x, cameraPosition.y, cameraPosition.z);

        for (uint32_t i = 0; i < lights.size(); i++)
        {
            std::string uniformName = "u_PointLights[" + std::to_string(i) + "]";
            const PointLight& light = lights[i];

            glUniform3f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Position"), light.Position.x, light.Position.y, light.Position.z);
            glUniform4f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Color"), light.Color.x, light.Color.y, light.Color.z, light.Color.w);
            glUniform1f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Intensity"), light.Intensity);
        }

        glBindVertexArray(m_VertexArrayID);
        glDrawElements(GL_TRIANGLES, (int)Indices.size(), GL_UNSIGNED_INT, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void Mesh::Draw(const TransformComponent& transform, const glm::vec3& cameraPosition, const std::vector<PointLight>& lights, const Material& material, int entityID)
    {
        if (m_EntityID != entityID)
        {
            m_EntityID = entityID;
            for (auto& vertex : Vertices)
                vertex.entityID = m_EntityID;
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, Vertices.size() * sizeof(OpenGLVertex), Vertices.data());

        for (uint16_t i = 0; i < TextureIDs.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, TextureIDs[i]);
        }

        glUseProgram(m_ShaderProgramID);
        glUniformMatrix4fv(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_ModelMatrix"), 1, GL_FALSE, glm::value_ptr(transform.GetTransform()));
        glUniform1i(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_LightCount"), (int)lights.size());
        glUniform3f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_CameraPosition"), cameraPosition.x, cameraPosition.y, cameraPosition.z);

        // Set Material
        glUniform3fv(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_Material.Albedo"), 1, glm::value_ptr(material.Albedo));
        glUniform1f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_Material.Roughness"), material.Roughness);
        glUniform1i(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_Material.IsMetal"), material.IsMetal);

        for (uint32_t i = 0; i < lights.size(); i++)
        {
            std::string uniformName = "u_PointLights[" + std::to_string(i) + "]";
            const PointLight& light = lights[i];

            glUniform3f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Position"), light.Position.x, light.Position.y, light.Position.z);
            glUniform4f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Color"), light.Color.x, light.Color.y, light.Color.z, light.Color.w);
            glUniform1f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Intensity"), light.Intensity);
        }

        glBindVertexArray(m_VertexArrayID);
        glDrawElements(GL_TRIANGLES, (int)Indices.size(), GL_UNSIGNED_INT, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void Mesh::Draw(const TransformComponent& transform, const glm::vec3& cameraPosition, const std::vector<PointLight>& lights, const DirectionalLight& directionalLight, const Material& material, int entityID)
    {
        if (m_EntityID != entityID)
        {
            m_EntityID = entityID;
            for (auto& vertex : Vertices)
                vertex.entityID = m_EntityID;
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, Vertices.size() * sizeof(OpenGLVertex), Vertices.data());

        for (uint16_t i = 0; i < TextureIDs.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, TextureIDs[i]);
        }

        glUseProgram(m_ShaderProgramID);
        glUniformMatrix4fv(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_ModelMatrix"), 1, GL_FALSE, glm::value_ptr(transform.GetTransform()));
        glUniform1i(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_LightCount"), (int)lights.size());
        glUniform3f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_CameraPosition"), cameraPosition.x, cameraPosition.y, cameraPosition.z);

        // Set Material
        glUniform3fv(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_Material.Albedo"), 1, glm::value_ptr(material.Albedo));
        glUniform1f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_Material.Roughness"), material.Roughness);
        glUniform1i(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_Material.IsMetal"), material.IsMetal);

        // Set Directional Light
        glUniform3f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_DirectionalLight.Direction"), directionalLight.Direction.x, directionalLight.Direction.y, directionalLight.Direction.z);
        glUniform4f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_DirectionalLight.Color"), directionalLight.Color.x, directionalLight.Color.y, directionalLight.Color.z, directionalLight.Color.w);
        glUniform1f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_DirectionalLight.Intensity"), directionalLight.Intensity);

        // Set Point Lights
        for (uint32_t i = 0; i < lights.size(); i++)
        {
            std::string uniformName = "u_PointLights[" + std::to_string(i) + "]";
            const PointLight& light = lights[i];

            glUniform3f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Position"), light.Position.x, light.Position.y, light.Position.z);
            glUniform4f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Color"), light.Color.x, light.Color.y, light.Color.z, light.Color.w);
            glUniform1f(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, uniformName + ".Intensity"), light.Intensity);
        }

        glBindVertexArray(m_VertexArrayID);
        glDrawElements(GL_TRIANGLES, (int)Indices.size(), GL_UNSIGNED_INT, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    Mesh::~Mesh()
    {
    }
}
