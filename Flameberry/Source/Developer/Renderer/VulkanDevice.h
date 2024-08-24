#pragma once

#include <vector>
#include <set>
#include <vulkan/vulkan.h>

#include "Core/Core.h"
#include "Renderer/VulkanWindow.h"

namespace Flameberry {

	struct QueueFamilyIndices
	{
		uint32_t GraphicsQueueFamilyIndex = -1;
		uint32_t ComputeQueueFamilyIndex = -1;
		uint32_t PresentationSupportedQueueFamilyIndex = -1;
	};

	class VulkanDevice
	{
	public:
		VulkanDevice(VkPhysicalDevice& physicalDevice, VulkanWindow* pVulkanWindow);
		~VulkanDevice();

		inline VkDevice GetVulkanDevice() const { return m_VulkanDevice; }
		inline VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		inline VkQueue GetComputeQueue() const { return m_ComputeQueue; }
		inline VkQueue GetPresentationQueue() const { return m_PresentationQueue; }
		inline QueueFamilyIndices GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
		VkCommandPool GetGraphicsCommandPool() const { return m_GraphicsQueueCommandPool; }
		VkCommandPool GetComputeCommandPool() const { return m_ComputeQueueCommandPool; }

		void BeginSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer, bool isCompute = false) const;
		void EndSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer, bool isCompute = false) const;

		void WaitIdle() const;
		void WaitIdleGraphicsQueue() const;
		void WaitIdleComputeQueue() const;

		std::vector<VkDeviceQueueCreateInfo> CreateDeviceQueueInfos(const std::set<uint32_t>& uniqueQueueFamilyIndices);

	private:
		VkDevice m_VulkanDevice;
		VkQueue m_GraphicsQueue, m_ComputeQueue, m_PresentationQueue;
		QueueFamilyIndices m_QueueFamilyIndices;

		VkCommandPool m_GraphicsQueueCommandPool, m_ComputeQueueCommandPool;

		VkPhysicalDevice& m_VulkanPhysicalDevice;
	};

} // namespace Flameberry
