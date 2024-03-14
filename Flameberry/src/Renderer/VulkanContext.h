#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "Core/Core.h"
#include "VulkanWindow.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "DescriptorSet.h"

namespace Flameberry {
    class VulkanContext
    {
    public:
        VulkanContext(VulkanWindow* pWindow);
        ~VulkanContext();

        static Ref<VulkanInstance> GetCurrentInstance() { return GetCurrentContext()->m_VulkanInstance; }
        static Ref<VulkanDevice> GetCurrentDevice() { return GetCurrentContext()->m_VulkanDevice; }
        static VkPhysicalDevice GetPhysicalDevice() { return GetCurrentContext()->m_VkPhysicalDevice; }
        static VkPhysicalDeviceProperties GetPhysicalDeviceProperties() {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(GetCurrentContext()->m_VkPhysicalDevice, &properties);
            return properties;
        }
        static VulkanWindow* GetCurrentWindow() { return GetCurrentContext()->m_Window; }
        static Ref<DescriptorPool> GetCurrentGlobalDescriptorPool() { return GetCurrentContext()->m_GlobalDescriptorPool; }
        static bool EnableValidationLayers() { return s_EnableValidationLayers; }

        static VkPhysicalDevice GetValidPhysicalDevice(const std::vector<VkPhysicalDevice>& vk_physical_devices, VkSurfaceKHR surface);
        static std::vector<const char*> GetValidationLayerNames() { return s_ValidationLayers; }

        static void SetCurrentContext(VulkanContext* pContext) { s_CurrentContext = pContext; }
        static VulkanContext* GetCurrentContext() {
            if (!s_CurrentContext)
                FBY_ERROR("Attempted to access current context which is null!");
            return s_CurrentContext;
        }
    private:
        Ref<VulkanInstance> m_VulkanInstance;
        Ref<VulkanDevice> m_VulkanDevice;
        VkPhysicalDevice m_VkPhysicalDevice = VK_NULL_HANDLE;
        VulkanWindow* m_Window = nullptr;

        Ref<DescriptorPool> m_GlobalDescriptorPool;
    private:
        static std::vector<const char*> s_VkDeviceExtensions;
        static const std::vector<const char*> s_ValidationLayers;
        static bool s_EnableValidationLayers;
    private:
        static VulkanContext* s_CurrentContext;
    };
}
