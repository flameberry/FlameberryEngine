#pragma once

#include <vulkan/vulkan.h>

namespace Flameberry {

	class VulkanQueue
	{
	public:
		VulkanQueue();
		~VulkanQueue() = default;

	private:
		VkQueue m_VulkanQueue;
	};

} // namespace Flameberry