#include "VulkanDevice.h"

#include "VulkanDebug.h"
#include "VulkanContext.h"
#include "RenderCommand.h"

namespace Flameberry {

	VulkanDevice::VulkanDevice(VkPhysicalDevice& physicalDevice, VulkanWindow* pVulkanWindow)
		: m_VulkanPhysicalDevice(physicalDevice)
	{
		// Getting Queue Family Indices
		m_QueueFamilyIndices = RenderCommand::GetQueueFamilyIndices(m_VulkanPhysicalDevice, pVulkanWindow->GetWindowSurface());

		std::vector<VkDeviceQueueCreateInfo> vulkanDeviceQueueCreateInfos = CreateDeviceQueueInfos({ m_QueueFamilyIndices.GraphicsQueueFamilyIndex,
			m_QueueFamilyIndices.ComputeQueueFamilyIndex,
			m_QueueFamilyIndices.PresentationSupportedQueueFamilyIndex });

		VkPhysicalDeviceFeatures2 deviceFeatures2{};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

		deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
		deviceFeatures2.features.sampleRateShading = VK_TRUE;
		deviceFeatures2.features.fillModeNonSolid = VK_TRUE;
		deviceFeatures2.features.tessellationShader = VK_TRUE;
		deviceFeatures2.features.depthClamp = VK_TRUE;

		VkPhysicalDeviceVulkan12Features vulkan12Features{};
		vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		vulkan12Features.descriptorIndexing = VK_TRUE;

		deviceFeatures2.pNext = &vulkan12Features;

		// Creating Vulkan Logical Device
		VkDeviceCreateInfo vk_device_create_info{};
		vk_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		vk_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(vulkanDeviceQueueCreateInfos.size());
		vk_device_create_info.pQueueCreateInfos = vulkanDeviceQueueCreateInfos.data();

		vk_device_create_info.pEnabledFeatures = nullptr;

		vk_device_create_info.enabledExtensionCount = static_cast<uint32_t>(VulkanContext::GetVulkanDeviceExtensions().size());
		vk_device_create_info.ppEnabledExtensionNames = VulkanContext::GetVulkanDeviceExtensions().data();

		// Enable multiview
		VkPhysicalDeviceMultiviewFeaturesKHR physicalDeviceMultiviewFeatures{};
		physicalDeviceMultiviewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
		physicalDeviceMultiviewFeatures.multiview = VK_TRUE;

		vk_device_create_info.pNext = &physicalDeviceMultiviewFeatures;

		physicalDeviceMultiviewFeatures.pNext = &deviceFeatures2;

		// Validation Layers
		auto validationLayers = VulkanContext::GetValidationLayerNames();
		if (VulkanContext::EnableValidationLayers())
		{
			vk_device_create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			vk_device_create_info.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			vk_device_create_info.enabledLayerCount = 0;
		}

		VK_CHECK_RESULT(vkCreateDevice(m_VulkanPhysicalDevice, &vk_device_create_info, nullptr, &m_VulkanDevice));

		vkGetDeviceQueue(m_VulkanDevice, m_QueueFamilyIndices.GraphicsQueueFamilyIndex, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_VulkanDevice, m_QueueFamilyIndices.ComputeQueueFamilyIndex, 0, &m_ComputeQueue);
		vkGetDeviceQueue(m_VulkanDevice, m_QueueFamilyIndices.PresentationSupportedQueueFamilyIndex, 0, &m_PresentationQueue);

		{
			// Creating Graphics Queue Command Pool
			VkCommandPoolCreateInfo commandPoolCreateInfo{};
			commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCreateInfo.queueFamilyIndex = m_QueueFamilyIndices.GraphicsQueueFamilyIndex;
			commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			VK_CHECK_RESULT(vkCreateCommandPool(m_VulkanDevice, &commandPoolCreateInfo, nullptr, &m_GraphicsQueueCommandPool));
		}

		{
			// Creating Compute Queue Command Pool
			VkCommandPoolCreateInfo commandPoolCreateInfo{};
			commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCreateInfo.queueFamilyIndex = m_QueueFamilyIndices.ComputeQueueFamilyIndex;
			commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		
			VK_CHECK_RESULT(vkCreateCommandPool(m_VulkanDevice, &commandPoolCreateInfo, nullptr, &m_ComputeQueueCommandPool));
		}
	}

	void VulkanDevice::BeginSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer, bool isCompute) const
	{
		VkCommandBufferAllocateInfo vk_command_buffer_allocate_info{};
		vk_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		vk_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vk_command_buffer_allocate_info.commandPool = isCompute ? m_ComputeQueueCommandPool : m_GraphicsQueueCommandPool;
		vk_command_buffer_allocate_info.commandBufferCount = 1;

		vkAllocateCommandBuffers(m_VulkanDevice, &vk_command_buffer_allocate_info, &commandBuffer);

		VkCommandBufferBeginInfo vk_command_buffer_begin_info{};
		vk_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vk_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &vk_command_buffer_begin_info);
	}

	void VulkanDevice::EndSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer, bool isCompute) const
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo vk_submit_info{};
		vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		vk_submit_info.commandBufferCount = 1;
		vk_submit_info.pCommandBuffers = &commandBuffer;

		VkQueue vulkanQueue = isCompute ? m_ComputeQueue : m_GraphicsQueue;
		vkQueueSubmit(vulkanQueue, 1, &vk_submit_info, VK_NULL_HANDLE);
		vkQueueWaitIdle(vulkanQueue);

		vkFreeCommandBuffers(m_VulkanDevice, m_GraphicsQueueCommandPool, 1, &commandBuffer);
	}

	VulkanDevice::~VulkanDevice()
	{
		vkDestroyCommandPool(m_VulkanDevice, m_GraphicsQueueCommandPool, nullptr);
		vkDestroyCommandPool(m_VulkanDevice, m_ComputeQueueCommandPool, nullptr);
		vkDestroyDevice(m_VulkanDevice, nullptr);
	}

	std::vector<VkDeviceQueueCreateInfo> VulkanDevice::CreateDeviceQueueInfos(const std::set<uint32_t>& uniqueQueueFamilyIndices)
	{
		constexpr float queuePriority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> vulkanDeviceQueueCreateInfos;
		for (uint32_t uniqueQueueFamilyIndex : uniqueQueueFamilyIndices)
		{
			VkDeviceQueueCreateInfo vulkanQueueCreateInfo{};
			vulkanQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			vulkanQueueCreateInfo.queueFamilyIndex = uniqueQueueFamilyIndex;
			vulkanQueueCreateInfo.queueCount = 1;
			vulkanQueueCreateInfo.pQueuePriorities = &queuePriority;
			vulkanDeviceQueueCreateInfos.push_back(vulkanQueueCreateInfo);
		}
		return vulkanDeviceQueueCreateInfos;
	}

	void VulkanDevice::WaitIdle() const
	{
		vkDeviceWaitIdle(m_VulkanDevice);
	}

	void VulkanDevice::WaitIdleGraphicsQueue() const
	{
		vkQueueWaitIdle(m_GraphicsQueue);
	}
	
	void VulkanDevice::WaitIdleComputeQueue() const
	{
		vkQueueWaitIdle(m_ComputeQueue);
	}

} // namespace Flameberry
