#pragma once

// Platform Specific Code
#ifdef FL_DEBUG
#ifdef __APPLE__
#define FL_DEBUGBREAK() abort()
#elif defined(WIN32) && defined(_MSVC_LANG)
#define FL_DEBUGBREAK() __debugbreak()
#endif
#else
#define FL_DEBUGBREAK()
#endif

namespace Flameberry {
    enum class RendererAPI { NONE = 0, OPENGL, VULKAN };
}

#define FL_RENDERER_API_NONE   Flameberry::RendererAPI::NONE
#define FL_RENDERER_API_OPENGL Flameberry::RendererAPI::OPENGL
#define FL_RENDERER_API_VULKAN Flameberry::RendererAPI::VULKAN

#ifdef FL_USE_OPENGL_API
#define FL_RENDERER_API_CURRENT Flameberry::RendererAPI::OPENGL
#elif defined FL_USE_VULKAN_API
#define FL_RENDERER_API_CURRENT Flameberry::RendererAPI::VULKAN
#else
#define FL_RENDERER_API_CURRENT Flameberry::RendererAPI::NONE
#endif

// Including All Utils related to Logging
#include "Log.h"

// Easy access to some important colors
#define FL_WHITE     glm::vec4{ 1.0f }
#define FL_YELLOW    glm::vec4{ 1.0f, 1.0f, 0.0f, 1.0f }
#define FL_PURPLE    glm::vec4{ 1.0f, 0.0f, 1.0f, 1.0f }
#define FL_BLUE      glm::vec4{ 0.0f, 1.0f, 1.0f, 1.0f }
#define FL_BLACK     glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f }
#define FL_DARK_BLUE glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f }
#define FL_RED       glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f }
#define FL_PINK      glm::vec4{ 1.0f, 157.0f / 255.0f, 207.0f / 255.0f, 1.0f }