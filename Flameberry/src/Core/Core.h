#pragma once

#include "PlatformDetection.h"

// Platform Specific Code
#ifdef FBY_DEBUG
    #ifdef FBY_PLATFORM_MACOS
        #define FBY_DEBUGBREAK() __builtin_trap()
    #elif defined(FBY_PLATFORM_WINDOWS) && defined(_MSVC_LANG)
        #define FBY_DEBUGBREAK() __debugbreak()
    #elif defined(FBY_PLATFORM_LINUX)
        #include <signal.h>
        #define FBY_DEBUGBREAK() raise(SIGTRAP)
    #else
        #error Platform doesn't support debugbreak yet!
    #endif

    #define FBY_ENABLE_ASSERTS
    #define FBY_CONFIG_STR "Debug"
#elif defined(FBY_RELEASE)
    #define FBY_CONFIG_STR "Release"
    #define FBY_DEBUGBREAK()
#else
    #define FBY_DEBUGBREAK()
#endif

#define FBY_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

// Including All Utils related to Logging
#include "Log.h"
#include "Assert.h"
