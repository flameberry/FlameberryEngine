#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK_RESULT(expr)                                                              \
	{                                                                                      \
		VkResult r = expr;                                                                 \
		FBY_ASSERT(r == VK_SUCCESS, "Vulkan process failed with: {}", string_VkResult(r)); \
	}
