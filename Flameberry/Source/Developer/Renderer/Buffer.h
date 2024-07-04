#pragma once

#include <memory>
#include <vulkan/vulkan.h>

#include "Core/Core.h"

namespace Flameberry {

	struct BufferSpecification
	{
		VkDeviceSize InstanceSize;
		uint32_t InstanceCount;
		VkBufferUsageFlags Usage;
		VkMemoryPropertyFlags MemoryProperties;
		VkDeviceSize MinOffsetAlignment = 0;
	};

	class Buffer
	{
	public:
		Buffer(const BufferSpecification& specification);
		~Buffer();

		void WriteToBuffer(const void* data, VkDeviceSize size,
			VkDeviceSize offset = 0);
		void WriteToIndex(const void* data, uint32_t index);
		VkResult Flush(VkDeviceSize size, VkDeviceSize offset);
		VkResult FlushIndex(int index);
		VkResult MapMemory(VkDeviceSize size, VkDeviceSize offset = 0);
		void UnmapMemory();

		const void* GetMappedMemory() const { return m_VkBufferMappedMemory; }
		BufferSpecification GetSpecification() const { return m_BufferSpec; }
		const VkBuffer& GetVulkanBuffer() const { return m_VkBuffer; }
		VkDeviceSize GetBufferSize() const
		{
			return m_AlignmentSize * m_BufferSpec.InstanceCount;
		}

	private:
		VkDeviceSize GetAlignment(VkDeviceSize instanceSize,
			VkDeviceSize minOffsetAlignment);

	private:
		VkBuffer m_VkBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_VkBufferDeviceMemory = VK_NULL_HANDLE;
		void* m_VkBufferMappedMemory = nullptr;

		VkDeviceSize m_AlignmentSize;

		BufferSpecification m_BufferSpec;
	};

} // namespace Flameberry