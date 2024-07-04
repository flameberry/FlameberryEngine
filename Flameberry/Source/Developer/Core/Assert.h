#pragma once

#ifdef FBY_ENABLE_ASSERTS
    #define FBY_DO_ON_ASSERT(x, ...) {if(!(x)) {__VA_ARGS__;}}
    #define FBY_ASSERT(x, ...) FBY_DO_ON_ASSERT(x, Flameberry::Logger::GetCoreLogger()->log_assert(__FILE__, __LINE__, __VA_ARGS__), FBY_DEBUGBREAK())
#else
    #define FBY_DO_ON_ASSERT(x, ...)
    #define FBY_ASSERT(x, ...)
#endif
