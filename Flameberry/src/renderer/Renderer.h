#pragma once
#include <array>
#include <vector>
#include <string>
#include <glad/glad.h>
#include <unordered_map>
#include <GLFW/glfw3.h>
#include "../core/Core.h"
#include <glm/glm.hpp>

#define FL_ELEMENT_TYPE_GENERAL_INDEX 0.0f
#define FL_ELEMENT_TYPE_PANEL_INDEX 1.0f
#define FL_ELEMENT_TYPE_BUTTON_INDEX 2.0f

/// This Macro contains the max number of texture slots that the GPU supports, varies for each computer.
#define MAX_TEXTURE_SLOTS 16
#define MAX_QUADS 1000
#define MAX_VERTICES 4 * MAX_QUADS
#define MAX_INDICES 6 * MAX_QUADS
#define TITLE_BAR_HEIGHT 15

namespace Flameberry {
    enum class UnitType
    {
        NONE = 0, PIXEL_UNITS, OPENGL_UNITS
    };

    struct RendererInitInfo
    {
        GLFWwindow* userWindow;
        bool enableFontRendering{ true };
        std::string fontFilePath{ FL_PROJECT_DIR"Flameberry/resources/fonts/OpenSans-Regular.ttf" };
    };


    // The [Vertex] struct represents an OpenGL Vertex.
    struct Vertex
    {
        /// Position from -1.0f to 1.0f on both x-axis and y-axis
        glm::vec3 position;
        /// Color in rgba format, each channel ranging from 0.0f to 1.0f
        glm::vec4 color;
        /// Texture coordinates ranging from 0.0f to 1.0f
        glm::vec2 texture_uv;
        /// Texture index which will be used as opengl texture slot to which the texture will be bound
        float     texture_index;
        /// Quad Dimensions which will be used by the shader to customize the quad
        glm::vec2 quad_dimensions;
        /// This will tell the shader what kind of UI Element is being rendered
        float     element_type_index;
        /// This will be used by the shader to determine the title bar color of the panel
        float     is_panel_active;

        /// Default Constructor
        Vertex()
            : position(0.0f), color(1.0f), texture_uv(0.0f), texture_index(-1.0f), quad_dimensions(0.0f), element_type_index(FL_ELEMENT_TYPE_GENERAL_INDEX)
        {
        }
    };

    class Renderer
    {
    public:
        // The Init function should be called after the GLFW window creation and before the main loop
        static void        Init(const RendererInitInfo& rendererInitInfo);
        static void        OnResize();
        static glm::vec2   GetWindowContentScale() { return S_WindowContentScale; }
        static GLFWwindow* GetUserGLFWwindow();
        static glm::vec2   GetViewportSize();
        static glm::vec2   GetTextDimensions(const std::string& text, float scale);
        static GLint       GetUniformLocation(const std::string& name, uint32_t shaderId);
        static float       GetAspectRatio() { return S_AspectRatio; }
        static glm::vec2   ConvertPixelsToOpenGLValues(const glm::vec2& value_in_pixels);
        static glm::vec2   ConvertOpenGLValuesToPixels(const glm::vec2& opengl_coords);
        static float       ConvertXAxisPixelValueToOpenGLValue(int X);
        static float       ConvertYAxisPixelValueToOpenGLValue(int Y);
        static void        AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const glm::vec4& color, const float elementTypeIndex, const char* textureFilePath, UnitType unitType = UnitType::PIXEL_UNITS, bool isPanelActive = false);
        static void        AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const glm::vec4& color, const float elementTypeIndex, UnitType unitType = UnitType::PIXEL_UNITS, bool isPanelActive = false);
        static void        AddText(const std::string& text, const glm::vec2& position_in_pixels, float scale, const glm::vec4& color);
        static uint32_t    CreateTexture(const std::string& filePath);
        static void        CleanUp();

        static void Begin();
        static void End();

        static glm::vec2& GetCursorPosition();
        static std::tuple<std::string, std::string> ReadShaderSource(const std::string& filePath);
    private:
        /// Batch Handling functions
        static void InitBatch();
        static void FlushBatch();

        /// Private Functions which will be used by the Renderer as Utilites
        static void     OnUpdate();
        static uint32_t GetTextureIdIfAvailable(const char* textureFilePath);
    private:
        /// Struct that contains all the matrices needed by the shader, which will be stored in a Uniform Buffer
        struct UniformBufferData { glm::mat4 ProjectionMatrix; };
        struct TextureUniformBufferData { int Samplers[MAX_TEXTURE_SLOTS]; };
        struct FontProps
        {
            float Scale;
            float Strength;
            float PixelRange;
            float AscenderY;
            float DescenderY;
        };
        struct Batch
        {
            /// Renderer IDs required for OpenGL 
            uint32_t VertexBufferId, IndexBufferId, VertexArrayId, ShaderProgramId;
            std::vector<uint32_t> TextureIds;
            /// All the vertices stored by a Batch.
            std::vector<Vertex> Vertices;
        };
        struct Character
        {
            uint32_t   textureId;  // ID handle of the glyph texture
            glm::vec2  size;       // Size of glyph
            glm::vec2  bearing;    // Offset from baseline to left/top of glyph
            double     advance;    // Offset to advance to next glyph
        };
    public:
        static FontProps& GetFontProps() { return S_FontProps; }
    private:
        /// Stores the window content scale, useful for correct scaling on retina displays
        static glm::vec2                                 S_WindowContentScale;
        /// AspectRatio used for converting pixel coordinates to opengl coordinates
        static float                                     S_AspectRatio;
        /// The RendererId needed for the Uniform Buffer
        static uint32_t                                  S_UniformBufferId;
        /// Contains Font properties of the main UI font
        static FontProps                                 S_FontProps;
        /// Stores all matrices needed by the shader, also stored in a Uniform Buffer
        static UniformBufferData                         S_UniformBufferData;
        /// Stores file path of the font provided by user and the default font file path
        static std::string                               S_UserFontFilePath;
        /// The main vector of all the batches of quads to ever exist in the program
        static Batch                                     S_Batch;
        /// Stores all the characters and their properties, which are extracted from the font provided by the user
        static std::unordered_map<char, Character>       S_Characters;
        /// Stores the size of the vieport that Flameberry is being drawn on
        static glm::vec2                                 S_ViewportSize;
        /// Stores the cursor position per frame on the User Window
        static glm::vec2                                 S_CursorPosition;
        /// Stores the GLFWwindow where Flameberry is being drawn
        static GLFWwindow* S_UserWindow;
        /// Stores the uniform location in a shader if the location needs to be reused
        static std::unordered_map<std::string, GLint>    S_UniformLocationCache;
        /// Stores the texture IDs of the already loaded textures to be reused
        static std::unordered_map<std::string, uint32_t> S_TextureIdCache;

        static float S_CurrentTextureSlot;

        constexpr static glm::vec4 S_TemplateVertexPositions[4] = {
             {-0.5f, -0.5f, 0.0f, 1.0f},
             {-0.5f,  0.5f, 0.0f, 1.0f},
             { 0.5f,  0.5f, 0.0f, 1.0f},
             { 0.5f, -0.5f, 0.0f, 1.0f}
        };
    };
}
