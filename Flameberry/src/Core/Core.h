#pragma once

// Platform Specific Code
#ifdef FBY_DEBUG
#ifdef __APPLE__
#define FBY_DEBUGBREAK() abort()
#elif defined(WIN32) && defined(_MSVC_LANG)
#define FBY_DEBUGBREAK() __debugbreak()
#endif
#else
#define FBY_DEBUGBREAK()
#endif

#ifdef FBY_DEBUG
#define FBY_CONFIG_STR "Debug"
#elif defined(FBY_RELEASE)
#define FBY_CONFIG_STR "Release"
#else
#define FBY_CONFIG_STR
#endif

#define FBY_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

// Including All Utils related to Logging
#include "Log.h"

// Easy access to some important colors
#define FBY_WHITE     glm::vec4{ 1.0f }
#define FBY_YELLOW    glm::vec4{ 1.0f, 1.0f, 0.0f, 1.0f }
#define FBY_PURPLE    glm::vec4{ 1.0f, 0.0f, 1.0f, 1.0f }
#define FBY_BLUE      glm::vec4{ 0.0f, 1.0f, 1.0f, 1.0f }
#define FBY_BLACK     glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f }
#define FBY_DARK_BLUE glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f }
#define FBY_RED       glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f }
#define FBY_PINK      glm::vec4{ 1.0f, 157.0f / 255.0f, 207.0f / 255.0f, 1.0f }
