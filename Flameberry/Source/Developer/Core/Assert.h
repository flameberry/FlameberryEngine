#pragma once

#undef FBY_ENABLE_ASSERTS

#ifdef FBY_ENABLE_ASSERTS
	#define FBY_INTERNAL_DO_ON_ASSERT(x, ...) \
		{                                     \
			if (!(x))                         \
			{                                 \
				__VA_ARGS__;                  \
			}                                 \
		}

	#define FBY_INTERNAL_ASSERT_IMPL(x, ...)                                                    \
		FBY_INTERNAL_DO_ON_ASSERT(x,                                                            \
			Flameberry::Logger::GetCoreLogger()->log_assert(__FILE__, __LINE__, ##__VA_ARGS__), \
			FBY_DEBUGBREAK())

	#define FBY_ASSERT(...) FBY_INTERNAL_ASSERT_IMPL(__VA_ARGS__)
#else
	#define FBY_INTERNAL_DO_ON_ASSERT(x, ...)
	#define FBY_ASSERT(x, ...)
#endif
