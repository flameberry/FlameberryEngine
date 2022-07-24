#pragma once

// Platform Dependent Code
#ifdef FL_DEBUG
#ifdef __APPLE__
#define FL_DEBUGBREAK() abort()
#elif defined(WIN32)
#define FL_DEBUGBREAK() __debugbreak()
#endif
#else
#define FL_DEBUGBREAK()
#endif

// Including All Utils related to Logging
#include "Log.h"

#ifdef FL_DEBUG

#include <glad/glad.h>
#include <string>

namespace Flameberry {
    /// Returns the appropriate OpenGL Error message
    static std::string GetErrorString(GLenum error)
    {
        switch (error)
        {
        case GL_INVALID_ENUM:                  return "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 return "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             return "INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:                return "STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:               return "STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:                 return "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "INVALID_FRAMEBUFFER_OPERATION"; break;
        default: return 0; break;
        }
    }

    /// Prints the OpenGL error message if error found
    static void GL_Error()
    {
        while (GLenum error = glGetError())
            FL_ERROR("[OpenGL Error]: {0}", GetErrorString(error));
    }
}

#define GL_ERROR() while (GLenum error = glGetError()) FL_ERROR("[OpenGL Error]: {0}", Flameberry::GetErrorString(error))

#define GL_CHECK_ERROR(x) while (glGetError() != GL_NO_ERROR);\
x;\
GL_ERROR()

#else
#define GL_CHECK_ERROR(x) x
#endif

namespace Flameberry {
    enum class QuadPosType { None = 0, QuadPosBottomLeftVertex, QuadPosCenter };
    struct Rect2D
    {
        float l, r, b, t;
        Rect2D() = default;
        Rect2D(float val)
            : l(val), r(val), b(val), t(val)
        {
        }
        Rect2D(float l, float r, float b, float t)
            : l(l), r(r), b(b), t(t)
        {
        }
    };
}

#define FL_QUAD_POS_BOTTOM_LEFT_VERTEX Flameberry::QuadPosType::QuadPosBottomLeftVertex
#define FL_QUAD_POS_CENTER Flameberry::QuadPosType::QuadPosCenter

// Easy access to some important colors
#define FL_WHITE glm::vec4{ 1.0f }
#define FL_YELLOW glm::vec4{ 1.0f, 1.0f, 0.0f, 1.0f }
#define FL_PURPLE glm::vec4{ 1.0f, 0.0f, 1.0f, 1.0f }
#define FL_BLUE glm::vec4{ 0.0f, 1.0f, 1.0f, 1.0f }
#define FL_BLACK glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f }
#define FL_DARK_BLUE glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f }
#define FL_RED glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f }
#define FL_PINK glm::vec4{ 1.0f, 157.0f / 255.0f, 207.0f / 255.0f, 1.0f }