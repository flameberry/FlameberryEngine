#include "CommandBuffer.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"

namespace Flameberry {

	CommandBuffer::CommandBuffer(const CommandBufferSpecification& specification)
		: m_Specification(specification)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.sType =
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandBufferCount = 1;
		commandBufferAllocateInfo.commandPool = m_Specification.CommandPool;
		commandBufferAllocateInfo.level = m_Specification.IsPrimary
			? VK_COMMAND_BUFFER_LEVEL_PRIMARY
			: VK_COMMAND_BUFFER_LEVEL_SECONDARY;

		const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo,
			&m_CommandBuffer));
	}

	CommandBuffer::~CommandBuffer() {}

	void CommandBuffer::Begin()
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo{};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags =
			m_Specification.SingleTimeUsage
			? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
			: 0;

		VK_CHECK_RESULT(
			vkBeginCommandBuffer(m_CommandBuffer, &commandBufferBeginInfo));
	}

	void CommandBuffer::End()
	{
		VK_CHECK_RESULT(vkEndCommandBuffer(m_CommandBuffer));
	}

	void CommandBuffer::Reset()
	{
		VK_CHECK_RESULT(vkResetCommandBuffer(m_CommandBuffer, 0));
	}

} // namespace Flameberry