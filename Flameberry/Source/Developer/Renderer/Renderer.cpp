#include "Renderer.h"

#include <glm/gtc/type_ptr.hpp>

#include "Asset/Importers/TextureImporter.h"
#include "Core/Application.h"
#include "Core/Core.h"
#include "Core/Profiler.h"

#include "VulkanContext.h"
#include "Asset/AssetManager.h"
#include "SwapChain.h"
#include "ShaderLibrary.h"
#include "Texture2D.h"
#include "MaterialAsset.h"
#include "Skymap.h"
#include "Font.h"

// #define FBY_ENABLE_QUERY_TIMESTAMP

namespace Flameberry {

	std::vector<Renderer::Command> Renderer::s_CommandQueue;
	uint32_t Renderer::s_RT_FrameIndex = 0, Renderer::s_FrameIndex = 0;
	RendererFrameStats Renderer::s_RendererFrameStats;
	std::array<Ref<CommandBuffer>, SwapChain::MAX_FRAMES_IN_FLIGHT> Renderer::s_CommandBuffers;

	VkQueryPool Renderer::s_QueryPool;
	std::array<uint64_t, 4 * SwapChain::MAX_FRAMES_IN_FLIGHT> Renderer::s_Timestamps;
	Ref<Texture2D> Renderer::s_CheckerboardTexture;

	void Renderer::Init()
	{
		// Create the generic texture descriptor layout
		Texture2D::InitStaticResources();
		Skymap::Init();
		ShaderLibrary::Init();

		{
			// The main command buffers
			CommandBufferSpecification cmdBufferSpec;
			cmdBufferSpec.CommandPool = VulkanContext::GetCurrentDevice()->GetGraphicsCommandPool();
			cmdBufferSpec.IsPrimary = true;
			cmdBufferSpec.SingleTimeUsage = false;

			// Create command buffers
			for (auto& commandBuffer : s_CommandBuffers)
				commandBuffer = CreateRef<CommandBuffer>(cmdBufferSpec);
		}

		s_CommandQueue.reserve(5 * 1028 * 1028 / sizeof(Renderer::Command)); // 5 MB

		// Load Generic Resources
		s_CheckerboardTexture = TextureImporter::LoadTexture2D(FBY_PROJECT_DIR "Flameberry/Assets/Icons/Checkerboard.png");

#ifdef FBY_ENABLE_QUERY_TIMESTAMP

		// Creating the query pool for recording GPU timestamps
		VkQueryPoolCreateInfo queryPoolCreateInfo = {};
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount = s_Timestamps.size();

		VK_CHECK_RESULT(vkCreateQueryPool(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &s_QueryPool));

#endif
	}

	void Renderer::Shutdown()
	{
#ifdef FBY_ENABLE_QUERY_TIMESTAMP

		vkDestroyQueryPool(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), s_QueryPool, nullptr);

#endif

		// Destroy Generic Resources
		s_CheckerboardTexture = nullptr;

		Font::DestroyDefault();
		ShaderLibrary::Shutdown();
		Skymap::Destroy();
		Texture2D::DestroyStaticResources();

		DescriptorSetLayout::ClearCache(); // TODO: Maybe move this to somewhere obvious like VulkanDevice or Renderer
	}

	void Renderer::WaitAndRender()
	{
		// Update the Frame Index of the Main Thread
		s_FrameIndex = (s_FrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
		RT_RenderFrame();
	}

	void Renderer::RT_RenderFrame()
	{
		FBY_PROFILE_SCOPE("RT_RenderLoop");
		auto& window = Application::Get().GetWindow();
		const auto& device = VulkanContext::GetCurrentDevice();

		// Execute all Render Commands
		if (window.BeginFrame())
		{
			s_CommandBuffers[s_RT_FrameIndex]->Reset();
			s_CommandBuffers[s_RT_FrameIndex]->Begin();

			const uint32_t imageIndex = window.GetImageIndex();

#ifdef FBY_ENABLE_QUERY_TIMESTAMP

			vkCmdResetQueryPool(commandBuffer, s_QueryPool, s_RT_FrameIndex * 2, 2);
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, s_QueryPool, s_RT_FrameIndex * 2);

#endif

			for (auto& cmd : s_CommandQueue)
				cmd(s_CommandBuffers[s_RT_FrameIndex]->GetVulkanCommandBuffer(), imageIndex);

#ifdef FBY_ENABLE_QUERY_TIMESTAMP

			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, s_QueryPool, s_RT_FrameIndex * 2 + 1);

#endif

			s_CommandBuffers[s_RT_FrameIndex]->End();
			window.SwapBuffers();
		}

#ifdef FBY_ENABLE_QUERY_TIMESTAMP

		QueryTimestampResults();

#endif

		ResetStats();
		s_CommandQueue.clear();

		// Update the Frame Index of the Render Thread
		s_RT_FrameIndex = (s_RT_FrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	/// @brief Obsolete: This function is used to render a single mesh by individually binding it's resources.
	/// It shouldn't be preferred anymore, as redundant bindings are a problem using this.
	/// @param mesh - The mesh to be rendered
	/// @param pipeline - The mesh pipeline
	/// @param materialTable - The corresponding material table
	/// @param transform - The transform matrix
	void Renderer::SubmitMeshWithMaterial(const Ref<StaticMesh>& mesh, const Ref<Pipeline>& pipeline, const MaterialTable& materialTable, const glm::mat4& transform)
	{
		Renderer::Submit([mesh, pipelineLayout = pipeline->GetVulkanPipelineLayout(), transform](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
			{
				Renderer::RT_BindVertexAndIndexBuffers(cmdBuffer, mesh->GetVertexBuffer()->GetVulkanBuffer(), mesh->GetIndexBuffer()->GetVulkanBuffer());
				vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(transform), glm::value_ptr(transform));
			});

		uint32_t submeshIndex = 0;
		for (const auto& submesh : mesh->GetSubMeshes())
		{
			// The Assumption here is every mesh loaded will have a Material, i.e. materialAsset won't be nullptr
			Ref<MaterialAsset> materialAsset;
			if (auto it = materialTable.find(submeshIndex); it != materialTable.end())
				materialAsset = AssetManager::GetAsset<MaterialAsset>(it->second);
			else if (AssetManager::IsAssetHandleValid(submesh.MaterialHandle))
				materialAsset = AssetManager::GetAsset<MaterialAsset>(submesh.MaterialHandle);

			Renderer::Submit([pipelineLayout = pipeline->GetVulkanPipelineLayout(), materialAsset, submesh = mesh->GetSubMeshes()[submeshIndex]](VkCommandBuffer cmdBuffer, uint32_t)
				{
					RT_BindMaterial(cmdBuffer, pipelineLayout, materialAsset->GetUnderlyingMaterial());
					vkCmdDrawIndexed(cmdBuffer, submesh.IndexCount, 1, submesh.IndexOffset, 0, 0);
				});
			submeshIndex++;

			// Record Statistics
			s_RendererFrameStats.SubMeshCount++;
			s_RendererFrameStats.DrawCallCount++;
			s_RendererFrameStats.IndexCount += submesh.IndexCount;
		}
		s_RendererFrameStats.MeshCount++;
	}

	void Renderer::RT_BindMaterial(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, const Ref<Material>& material)
	{
		vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, material->GetPushConstantOffset(), material->GetUniformDataSize(), material->GetUniformDataPtr());

		if (material->m_DescriptorSets.size())
		{
			int idx = 0;
			std::vector<VkDescriptorSet> descSetArray(material->m_DescriptorSets.size());

			for (const auto& set : material->m_DescriptorSets)
			{
				descSetArray[idx] = set->GetVulkanDescriptorSet();
				idx++;
			}

			const uint32_t setNumber = material->m_StartSetIndex;
			const uint32_t count = idx;

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, setNumber, count, &descSetArray[0], 0, nullptr);
		}

		// Record Statistics
		s_RendererFrameStats.BoundMaterials++;
	}

	void Renderer::RT_BindVertexAndIndexBuffers(VkCommandBuffer cmdBuffer, VkBuffer vertexBuffer, VkBuffer indexBuffer)
	{
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		s_RendererFrameStats.VertexAndIndexBufferStateSwitches++;
	}

	void Renderer::RT_BindPipeline(VkCommandBuffer cmdBuffer, VkPipeline pipeline)
	{
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	}

	void Renderer::ResetStats()
	{
		s_RendererFrameStats.MeshCount = 0;
		s_RendererFrameStats.SubMeshCount = 0;
		s_RendererFrameStats.BoundMaterials = 0;
		s_RendererFrameStats.DrawCallCount = 0;
		s_RendererFrameStats.IndexCount = 0;
		s_RendererFrameStats.VertexAndIndexBufferStateSwitches = 0;
	}

	void Renderer::QueryTimestampResults()
	{
		vkGetQueryPoolResults(
			VulkanContext::GetCurrentDevice()->GetVulkanDevice(),
			s_QueryPool,
			s_RT_FrameIndex * 2,
			2,
			2 * sizeof(uint64_t),
			&s_Timestamps[s_RT_FrameIndex * 2],
			sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

		uint64_t startTime = s_Timestamps[s_RT_FrameIndex * 2];
		uint64_t endTime = s_Timestamps[s_RT_FrameIndex * 2 + 1];
		uint64_t duration = endTime - startTime;

		// Convert the duration to milliseconds (or the desired unit)
		VkPhysicalDeviceProperties properties = VulkanContext::GetPhysicalDeviceProperties();

		double timestampPeriod = properties.limits.timestampPeriod;
		double gpuTime = duration * timestampPeriod / 1e6; // Convert to milliseconds

		FBY_LOG("GPU Time: {} ms", gpuTime);
	}

} // namespace Flameberry
