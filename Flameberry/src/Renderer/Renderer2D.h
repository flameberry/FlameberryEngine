#pragma once

#include <unordered_map>
#include <vector>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Core/Core.h"
#include "OrthographicCamera.h"

/// This Macro contains the max number of texture slots that the GPU supports, varies for each computer.
#define MAX_TEXTURE_SLOTS 16
#define MAX_QUADS 1000
#define MAX_VERTICES 4 * MAX_QUADS
#define MAX_INDICES 6 * MAX_QUADS

namespace Flameberry {
    struct Renderer2DInitInfo
    {
        GLFWwindow* userWindow;
        bool enableCustomViewport{ false };
        glm::vec2 customViewportSize;
    };

    // The [Vertex2D] struct represents an OpenGL Vertex.
    struct Vertex2D
    {
        glm::vec3 position;   // Position from -1.0f to 1.0f on both x-axis and y-axis
        glm::vec4 color;      // Color in rgba format, each channel ranging from 0.0f to 1.0f
        glm::vec2 texture_uv; // Texture coordinates ranging from 0.0f to 1.0f
        float texture_index;  // Texture index which will be used as opengl texture slot to which the texture will be bound

        // Default Constructor
        Vertex2D()
            : position(0.0f), color(1.0f), texture_uv(0.0f), texture_index(-1.0f)
        {}
    };

    class Renderer2D
    {
    public:
        Renderer2D();
        ~Renderer2D();

        // The Init function should be called after the GLFW window creation and before the main loop
        void        Init(const Renderer2DInitInfo& rendererInitInfo);
        glm::vec2   GetWindowContentScale() { return m_WindowContentScale; }
        GLFWwindow* GetUserGLFWwindow();
        glm::vec2   GetViewportSize();
        GLint       GetUniformLocation(const std::string& name, uint32_t shaderId);
        float       GetAspectRatio() { return m_AspectRatio; }
        void        AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const char* textureFilePath);
        void        AddQuad(const glm::mat4& transform, const glm::vec4& color);
        void        AddQuad(const glm::mat4& transform, const char* textureFilePath);
        void        AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const glm::vec4& color);
        void        CleanUp();
        void        Begin(OrthographicCamera& camera);
        void        End();
        void        SetCustomViewportSize(const glm::vec2& customViewportSize) { m_RendererInitInfo.customViewportSize = customViewportSize; }

        glm::vec2& GetCursorPosition();
        static std::shared_ptr<Renderer2D> Create() { return std::make_shared<Renderer2D>(); }
    private:
        /// Batch Handling functions
        void InitBatch();
        void FlushBatch();

        /// Private Functions which will be used by the Renderer as Utilites
        void     OnResize();
        void     OnUpdate();
        uint32_t GetTextureIdIfAvailable(const char* textureFilePath);

        void UpdateViewportSize();
        void UpdateWindowContentScale();
    private:
        /// Struct that contains all the matrices needed by the shader, which will be stored in a Uniform Buffer
        struct UniformBufferData { glm::mat4 ViewProjectionMatrix; };
        struct TextureUniformBufferData { int Samplers[MAX_TEXTURE_SLOTS]; };
        struct Batch
        {
            /// Renderer IDs required for OpenGL 
            uint32_t VertexBufferId, IndexBufferId, VertexArrayId, ShaderProgramId;
            std::vector<uint32_t> TextureIds;
            /// All the vertices stored by a Batch.
            std::vector<Vertex2D> Vertices;
        };
    private:
        Renderer2DInitInfo                        m_RendererInitInfo;
        // Stores the window content scale, useful for correct scaling on retina displays
        glm::vec2                                 m_WindowContentScale;
        /// AspectRatio used for converting pixel coordinates to opengl coordinates
        float                                     m_AspectRatio;
        /// The RendererId needed for the Uniform Buffer
        uint32_t                                  m_UniformBufferId;
        /// Stores all matrices needed by the shader, also stored in a Uniform Buffer
        UniformBufferData                         m_UniformBufferData;
        /// The main vector of all the batches of quads to ever exist in the program
        Batch                                     m_Batch;
        /// Stores the size of the vieport that Flameberry is being drawn on
        glm::vec2                                 m_ViewportSize;
        /// Stores the cursor position per frame on the User Window
        glm::vec2                                 m_CursorPosition;
        /// Stores the GLFWwindow where Flameberry is being drawn
        GLFWwindow* m_UserWindow;
        /// Stores the uniform location in a shader if the location needs to be reused
        std::unordered_map<std::string, GLint>    m_UniformLocationCache;
        /// Stores the texture IDs of the already loaded textures to be reused
        std::unordered_map<std::string, uint32_t> m_TextureIdCache;
        float m_CurrentTextureSlot;

        const glm::vec4 m_TemplateVertexPositions[4] = {
             {-0.5f, -0.5f, 0.0f, 1.0f},
             {-0.5f,  0.5f, 0.0f, 1.0f},
             { 0.5f,  0.5f, 0.0f, 1.0f},
             { 0.5f, -0.5f, 0.0f, 1.0f}
        };

        const glm::vec4 m_DefaultColor = glm::vec4(1);
    };
}
