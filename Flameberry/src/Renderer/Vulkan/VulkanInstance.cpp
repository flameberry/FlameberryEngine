#include "VulkanInstance.h"

#include <vector>
#include <GLFW/glfw3.h>

#include "Core/Core.h"
#include "VulkanDebug.h"
#include "VulkanContext.h"

namespace Flameberry {
    static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* userData
    )
    {
        switch (messageSeverity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            FL_LOG("{0}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            FL_INFO("{0}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            FL_WARN("{0}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            FL_ERROR("{0}", pCallbackData->pMessage);
            break;
        default:
            FL_TRACE("{0}", pCallbackData->pMessage);
            break;
        }
        return VK_FALSE;
    }

    VulkanInstance::VulkanInstance()
    {
        bool enableValidationLayers = VulkanContext::EnableValidationLayers();

        // Creating Vulkan Instance
        VkApplicationInfo vk_app_info{};
        vk_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        vk_app_info.pApplicationName = "Flameberry";
        vk_app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        vk_app_info.pEngineName = "No Engine";
        vk_app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        vk_app_info.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo vk_create_info{};
        vk_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        vk_create_info.pApplicationInfo = &vk_app_info;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> vk_extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        vk_extensions.push_back("VK_KHR_portability_enumeration");
        if (enableValidationLayers)
            vk_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        vk_create_info.enabledExtensionCount = static_cast<uint32_t>(vk_extensions.size());
        vk_create_info.ppEnabledExtensionNames = vk_extensions.data();
        vk_create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        VkDebugUtilsMessengerCreateInfoEXT vk_debug_create_info{};
        auto validationLayers = VulkanContext::GetValidationLayerNames();

        if (enableValidationLayers)
        {
            FL_LOG(validationLayers[0]);
            vk_create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            vk_create_info.ppEnabledLayerNames = validationLayers.data();

            vk_debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            vk_debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            vk_debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            vk_debug_create_info.pfnUserCallback = vk_debug_callback;

            vk_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&vk_debug_create_info;
        }
        else
        {
            vk_create_info.enabledLayerCount = 0;
            vk_create_info.pNext = nullptr;
        }

        VK_CHECK_RESULT(vkCreateInstance(&vk_create_info, nullptr, &m_VkInstance));
        FL_INFO("Created Vulkan Instance!");

        if (enableValidationLayers)
        {
            // Creating Vulkan Debug Messenger
            VkDebugUtilsMessengerCreateInfoEXT vk_debug_messenger_create_info{};
            vk_debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            vk_debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            vk_debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            vk_debug_messenger_create_info.pfnUserCallback = vk_debug_callback;
            vk_debug_messenger_create_info.pUserData = nullptr;

            VK_CHECK_RESULT(CreateDebugUtilsMessengerEXT(&vk_debug_messenger_create_info, nullptr, &m_VkDebugMessenger));
            FL_INFO("Created Vulkan Debug Messenger!");
        }
    }

    VkResult VulkanInstance::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger)
    {
        auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_VkInstance, "vkCreateDebugUtilsMessengerEXT");
        if (vkCreateDebugUtilsMessengerEXT)
            return vkCreateDebugUtilsMessengerEXT(m_VkInstance, pCreateInfo, pAllocator, pMessenger);
        else
            FL_ERROR("Failed to load function 'vkCreateDebugUtilsMessengerEXT'!");
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void VulkanInstance::DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
    {
        auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_VkInstance, "vkDestroyDebugUtilsMessengerEXT");
        if (vkDestroyDebugUtilsMessengerEXT)
            vkDestroyDebugUtilsMessengerEXT(m_VkInstance, messenger, pAllocator);
        else
            FL_ERROR("Failed to load function 'vkDestroyDebugUtilsMessengerEXT'!");
    }

    VulkanInstance::~VulkanInstance()
    {
        if (VulkanContext::EnableValidationLayers())
            DestroyDebugUtilsMessengerEXT(m_VkDebugMessenger, nullptr);
        vkDestroyInstance(m_VkInstance, nullptr);
    }
}
