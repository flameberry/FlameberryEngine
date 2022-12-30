#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "Core/Core.h"
#include "Core/optional.h"
#include "VulkanWindow.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"

namespace Flameberry {
    class VulkanContext
    {
    public:
        VulkanContext(VulkanWindow* pWindow);
        ~VulkanContext();

        static std::shared_ptr<VulkanContext> Create(VulkanWindow* pWindow) { return std::make_shared<VulkanContext>(pWindow); }

        static std::shared_ptr<VulkanInstance> GetCurrentInstance() { return GetCurrentContext()->m_VulkanInstance; }
        static std::shared_ptr<VulkanDevice> GetCurrentDevice() { return GetCurrentContext()->m_VulkanDevice; }
        static VkPhysicalDevice GetPhysicalDevice() { return GetCurrentContext()->m_VkPhysicalDevice; }
        static VkPhysicalDeviceProperties GetPhysicalDeviceProperties() {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(GetCurrentContext()->m_VkPhysicalDevice, &properties);
            return properties;
        }
        static VulkanWindow* GetCurrentWindow() { return GetCurrentContext()->m_Window; }
        static bool EnableValidationLayers() { return s_EnableValidationLayers; }

        static VkPhysicalDevice GetValidVkPhysicalDevice(const std::vector<VkPhysicalDevice>& vk_physical_devices, VkSurfaceKHR surface);
        static std::vector<const char*> GetValidationLayerNames() { return s_ValidationLayers; }

        static void SetCurrentContext(VulkanContext* pContext) { s_CurrentContext = pContext; }
        static VulkanContext* GetCurrentContext() {
            if (!s_CurrentContext) FL_ERROR("Attempted to access current context which is null!");
            return s_CurrentContext;
        }
    private:
        std::shared_ptr<VulkanInstance> m_VulkanInstance;
        std::shared_ptr<VulkanDevice> m_VulkanDevice;
        VkPhysicalDevice m_VkPhysicalDevice = VK_NULL_HANDLE;
        VulkanWindow* m_Window = nullptr;

    private:
        static std::vector<const char*> s_VkDeviceExtensions;
        static const std::vector<const char*> s_ValidationLayers;
        static bool s_EnableValidationLayers;
    private:
        static VulkanContext* s_CurrentContext;
    };
}
