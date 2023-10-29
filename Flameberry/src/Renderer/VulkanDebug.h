#pragma once

#include <string>
#include <vulkan/vulkan.h>

#include "Core/Core.h"

namespace Flameberry {
    static std::string GetVkResult(VkResult result)
    {
        std::string errorString = "";
        switch (result)
        {
        case VK_SUCCESS: break;
        case VK_NOT_READY: errorString = "VK_NOT_READY"; break;
        case VK_TIMEOUT: errorString = "VK_TIMEOUT"; break;
        case VK_EVENT_SET: errorString = "VK_EVENT_SET"; break;
        case VK_EVENT_RESET: errorString = "VK_EVENT_RESET"; break;
        case VK_INCOMPLETE: errorString = "VK_INCOMPLETE"; break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: errorString = "VK_ERROR_OUT_OF_HOST_MEMORY"; break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: errorString = "VK_ERROR_OUT_OF_DEVICE_MEMORY"; break;
        case VK_ERROR_INITIALIZATION_FAILED: errorString = "VK_ERROR_INITIALIZATION_FAILED"; break;
        case VK_ERROR_DEVICE_LOST: errorString = "VK_ERROR_DEVICE_LOST"; break;
        case VK_ERROR_MEMORY_MAP_FAILED: errorString = "VK_ERROR_MEMORY_MAP_FAILED"; break;
        case VK_ERROR_LAYER_NOT_PRESENT: errorString = "VK_ERROR_LAYER_NOT_PRESENT"; break;
        case VK_ERROR_EXTENSION_NOT_PRESENT: errorString = "VK_ERROR_EXTENSION_NOT_PRESENT"; break;
        case VK_ERROR_FEATURE_NOT_PRESENT: errorString = "VK_ERROR_FEATURE_NOT_PRESENT"; break;
        case VK_ERROR_INCOMPATIBLE_DRIVER: errorString = "VK_ERROR_INCOMPATIBLE_DRIVER"; break;
        case VK_ERROR_TOO_MANY_OBJECTS: errorString = "VK_ERROR_TOO_MANY_OBJECTS"; break;
        case VK_ERROR_FORMAT_NOT_SUPPORTED: errorString = "VK_ERROR_FORMAT_NOT_SUPPORTED"; break;
        case VK_ERROR_FRAGMENTED_POOL: errorString = "VK_ERROR_FRAGMENTED_POOL"; break;
        case VK_ERROR_UNKNOWN: errorString = "VK_ERROR_UNKNOWN"; break;
        case VK_ERROR_OUT_OF_POOL_MEMORY: errorString = "VK_ERROR_OUT_OF_POOL_MEMORY"; break;
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: errorString = "VK_ERROR_INVALID_EXTERNAL_HANDLE"; break;
        case VK_ERROR_FRAGMENTATION: errorString = "VK_ERROR_FRAGMENTATION"; break;
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: errorString = "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS"; break;
        case VK_PIPELINE_COMPILE_REQUIRED: errorString = "VK_PIPELINE_COMPILE_REQUIRED"; break;
        case VK_ERROR_SURFACE_LOST_KHR: errorString = "VK_ERROR_SURFACE_LOST_KHR"; break;
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: errorString = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"; break;
        case VK_SUBOPTIMAL_KHR: errorString = "VK_SUBOPTIMAL_KHR"; break;
        case VK_ERROR_OUT_OF_DATE_KHR: errorString = "VK_ERROR_OUT_OF_DATE_KHR"; break;
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: errorString = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR"; break;
        case VK_ERROR_VALIDATION_FAILED_EXT: errorString = "VK_ERROR_VALIDATION_FAILED_EXT"; break;
        case VK_ERROR_INVALID_SHADER_NV: errorString = "VK_ERROR_INVALID_SHADER_NV"; break;
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: errorString = "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT"; break;
        case VK_ERROR_NOT_PERMITTED_KHR: errorString = "VK_ERROR_NOT_PERMITTED_KHR"; break;
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: errorString = "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT"; break;
        case VK_THREAD_IDLE_KHR: errorString = "VK_THREAD_IDLE_KHR"; break;
        case VK_THREAD_DONE_KHR: errorString = "VK_THREAD_DONE_KHR"; break;
        case VK_OPERATION_DEFERRED_KHR: errorString = "VK_OPERATION_DEFERRED_KHR"; break;
        case VK_OPERATION_NOT_DEFERRED_KHR: errorString = "VK_OPERATION_NOT_DEFERRED_KHR"; break;
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: errorString = "VK_ERROR_COMPRESSION_EXHAUSTED_EXT"; break;
        case VK_RESULT_MAX_ENUM: errorString = "VK_RESULT_MAX_ENUM"; break;
        }
        return errorString;
    }
}

#ifdef FL_DEBUG
#define VK_CHECK_RESULT(result) FL_DO_ON_ASSERT(result == VK_SUCCESS, Flameberry::Logger::CoreLogger->log_assert(__FILE__, __LINE__, "Vulkan process failed with: {0}", Flameberry::GetVkResult(result)), FL_DEBUGBREAK())
#else
#define VK_CHECK_RESULT(result) result
#endif
