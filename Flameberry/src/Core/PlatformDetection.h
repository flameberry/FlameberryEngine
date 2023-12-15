#pragma once

#if defined(_WIN32)
#define FBY_PLATFORM_WINDOWS

#elif defined(__linux__)
#define FBY_PLATFORM_LINUX

#elif defined(FBY_PLATFORM_MACOS) && defined(__MACH__)
#define FBY_PLATFORM_MACOS

#else
#error "Unsupported platform"
#endif