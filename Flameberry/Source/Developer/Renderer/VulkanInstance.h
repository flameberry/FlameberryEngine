#pragma once

#include <memory>
#include <vulkan/vulkan.h>

namespace Flameberry {
	class VulkanInstance
	{
	public:
		VulkanInstance();
		~VulkanInstance();

		VkInstance GetVulkanInstance() const { return m_VkInstance; }

		VkResult CreateDebugUtilsMessengerEXT(
			const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator,
			VkDebugUtilsMessengerEXT* pMessenger);
		void DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT messenger,
			const VkAllocationCallbacks* pAllocator);

	private:
		VkInstance m_VkInstance;
		VkDebugUtilsMessengerEXT m_VkDebugMessenger;
	};
} // namespace Flameberry
