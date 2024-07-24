#pragma once

#include <thread>
#include <mutex>
#include <functional>
#include <vulkan/vulkan.h>

#include "Material.h"
#include "Pipeline.h"
#include "Renderer/Texture2D.h"
#include "StaticMesh.h"
#include "CommandBuffer.h"
#include "ECS/Components.h"

namespace Flameberry {

	struct RendererFrameStats
	{
		uint32_t MeshCount = 0, SubMeshCount = 0, BoundMaterials = 0;
		uint32_t DrawCallCount = 0, IndexCount = 0;

		uint32_t VertexAndIndexBufferStateSwitches = 0;
	};

	class Renderer
	{
	public:
		using Command = std::function<void(VkCommandBuffer, uint32_t)>;

	public:
		// Initialize and Shutdown the resources of all rendering related classes
		static void Init();
		static void Shutdown();

		// This is a hot function which is most likely a bottleneck and cause of high CPU usage
		inline static void Submit(const Command&& cmd) { s_CommandQueue.push_back(cmd); }
		// Currently just renders, the `Wait` part is for a future implementation of Multi-threading
		static void WaitAndRender();

		// Get the Renderer Stats
		static const RendererFrameStats& GetRendererFrameStats() { return s_RendererFrameStats; }
		// Get the current frame index in the update/main thread
		static uint32_t GetCurrentFrameIndex() { return s_FrameIndex; }
		// Get the current frame index in the render thread
		static uint32_t RT_GetCurrentFrameIndex() { return s_RT_FrameIndex; }
		// The render function to be called in the Render Thread which does the actual rendering by execution of the submitted commands
		static void RT_RenderFrame();
		// Get the Vulkan Command Buffer that is being actively recorded
		static VkCommandBuffer GetActiveVulkanCommandBuffer() { return s_CommandBuffers[s_RT_FrameIndex]->GetVulkanCommandBuffer(); }

		// Rendering Utilities
		static void SubmitMeshWithMaterial(const Ref<StaticMesh>& mesh, const Ref<Pipeline>& pipeline, const MaterialTable& materialTable, const glm::mat4& transform);
		static void RT_BindPipeline(VkCommandBuffer cmdBuffer, VkPipeline pipeline);
		static void RT_BindMaterial(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, const Ref<Material>& material);
		static void RT_BindVertexAndIndexBuffers(VkCommandBuffer cmdBuffer, VkBuffer vertexBuffer, VkBuffer indexBuffer);

		// Retrieve Generic Resources
		static Ref<Texture2D> GetCheckerboardTexture() { return s_CheckerboardTexture; }

	private:
		static void ResetStats();
		static void QueryTimestampResults();

	private:
		static uint32_t s_RT_FrameIndex, s_FrameIndex;
		static std::array<Ref<CommandBuffer>, SwapChain::MAX_FRAMES_IN_FLIGHT> s_CommandBuffers;

		static RendererFrameStats s_RendererFrameStats;

		// Critical Variables
		static std::vector<Command> s_CommandQueue;

		// Query Pool
		static VkQueryPool s_QueryPool;
		static std::array<uint64_t, 4 * SwapChain::MAX_FRAMES_IN_FLIGHT> s_Timestamps;

	private:
		// Generic Resources required all over the application
		static Ref<Texture2D> s_CheckerboardTexture;
	};

} // namespace Flameberry
