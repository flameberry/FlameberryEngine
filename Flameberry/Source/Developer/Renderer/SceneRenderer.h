#pragma once

#include "CommandBuffer.h"
#include "DescriptorSet.h"
#include "Pipeline.h"
#include "MaterialAsset.h"
#include "ECS/Components.h"
#include "ECS/Scene.h"
#include "Renderer/SwapChain.h"
#include "FrameResource.h"
#include "vulkan/vulkan_core.h"

namespace Flameberry {

	struct ModelMatrixPushConstantData
	{
		glm::mat4 ModelMatrix;
	};

	struct MousePickingPushConstantData
	{
		glm::mat4 ModelMatrix;
		int EntityIndex;
	};

	struct SceneRendererSettings
	{
		bool FrustumCulling = true, ShowBoundingBoxes = false;
		float GammaCorrectionFactor = 2.2f, Exposure = 1.0f;

		bool EnableShadows = true, ShowCascades = false, SoftShadows = true, SkyReflections = true;
		float CascadeLambdaSplit = 0.91f;

		bool GridFading = true;
		float GridNear = 0.1f, GridFar = 100.0f;

		float SelectionOutlineWidth = 1.0f;

		// Bloom Settings
		bool EnableBloom = true;
		float BloomThreshold = 1.5f, BloomKnee = 0.1f, BloomSpreadScale = 1.0f;

		static constexpr uint32_t CascadeCount = 4,
								  CascadeSize = 1024 * 2; // TODO: Make this a renderer startup setting
	};

	struct Cascade
	{
		glm::mat4 ViewProjectionMatrix;
		float DepthSplit;
	};

	///////////////////////////////////////////////////////////////////////////////////
	///////// Data Structures for storing all Rendering Information Per Frame /////////

	struct RenderObject
	{
		VkBuffer VertexBuffer, IndexBuffer;
		uint32_t IndexOffset, IndexCount;

		TransformComponent* Transform;

		Ref<MaterialAsset> MaterialAsset;

		RenderObject(VkBuffer vertexBuffer, VkBuffer indexBuffer, uint32_t indexOffset, uint32_t indexCount, TransformComponent* transform, Ref<Flameberry::MaterialAsset> materialAsset)
			: VertexBuffer(vertexBuffer), IndexBuffer(indexBuffer), IndexOffset(indexOffset), IndexCount(indexCount), Transform(transform), MaterialAsset(materialAsset)
		{
		}
	};

	struct RendererData
	{
		std::vector<RenderObject> RenderObjects, SelectedRenderObjects;
	};

	///////////////////////////////////////////////////////////////////////////////////

	class SceneRenderer
	{
	public:
		SceneRenderer(const glm::vec2& viewportSize);
		~SceneRenderer();

		void RenderScene(const glm::vec2& viewportSize, const Ref<Scene>& scene, const GenericCamera& camera, const glm::vec3& cameraPosition, FEntity selectedEntity, bool renderGrid = true, bool renderDebugIcons = true, bool renderOutline = true, bool renderPhysicsCollider = true);
		void RenderScene(const glm::vec2& viewportSize, const Ref<Scene>& scene, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec3& cameraPosition, float cameraNear, float cameraFar, FEntity selectedEntity, bool renderGrid = true, bool renderDebugIcons = true, bool renderOutline = true, bool renderPhysicsCollider = true);

		VkImageView GetGeometryPassOutputImageView(uint32_t index) const { return m_GeometryPass->GetSpecification().TargetFramebuffers[index]->GetColorResolveAttachment(0)->GetVulkanImageView(); }
		VkImageView GetCompositePassOutputImageView(uint32_t index) const { return m_CompositingPass->GetSpecification().TargetFramebuffers[index]->GetColorAttachment(0)->GetVulkanImageView(); }

		SceneRendererSettings& GetRendererSettingsRef() { return m_RendererSettings; }
		void RenderSceneForMousePicking(const Ref<Scene>& scene, const Ref<RenderPass>& renderPass, const Ref<Pipeline>& pipeline, const Ref<Pipeline>& pipeline2D, const glm::vec2& mousePos);

		void ReloadMeshShaders();

	private:
		void Init();

		void SubmitRenderObjects(std::vector<RenderObject>& renderObjects);

		void CalculateShadowMapCascades(const glm::mat4& viewProjectionMatrix, float cameraNear, float cameraFar, const glm::vec3& lightDirection);

		// Debug Utilities
		void SubmitPhysicsColliderGeometry(const Ref<Scene>& scene, FEntity entity, TransformComponent& transform);
		void SubmitCameraViewGeometry(const Ref<Scene>& scene, FEntity entity, TransformComponent& transform);
		void SubmitBoxColliderGeometry(const BoxColliderComponent& boxCollider, const TransformComponent& transform, const glm::mat3& rotation, const glm::vec3& color, const float bias);
		void SubmitSphereColliderGeometry(const SphereColliderComponent& sphereCollider, const TransformComponent& transform, const glm::vec3& color, const float bias);
		void SubmitCapsuleColliderGeometry(const CapsuleColliderComponent& capsuleCollider, const TransformComponent& transform, const glm::mat3& rotation, const glm::vec3& color, const float bias);

		// Render Passes
		void PrepareShadowMappingRenderPass();
		void PrepareGeometryRenderPass();
		void PrepareBloomPass();
		void PrepareJumpFloodPass();
		void PrepareCompositeRenderPass();

		// As the bloom pass is dependent on the viewport size, the size of it's associated images needs to be updated
		// i.e., the images need to be resized and descriptors need to be updated
		void PrepareBloomImageAndDescriptors();
		void CreateBloomSampler(const uint32_t mipLevels);

		void InvalidateGeometryPass(const uint32_t resourceIndex, const glm::vec2& viewportSize);
		void InvalidateBloomPass(const uint32_t resourceIndex, const glm::vec2& newBloomImgSize);
		void InvalidateJumpFloodPass(const uint32_t resourceIndex, const glm::vec2& newJumpFloodImgSize);

		void BloomPass();
		void JumpFloodPass();
		void CompositingPass();

		// Pipelines
		void CreateMeshPipeline();
		void CreateSkymapPipeline();
		void CreateGridPipeline();

	private:
		glm::vec2 m_ViewportSize;

		// Command Buffers
		std::vector<Ref<CommandBuffer>> m_CommandBuffers;

		// Geometry
		Ref<RenderPass> m_GeometryPass;
		Ref<DescriptorSetLayout> m_CameraBufferDescSetLayout, m_SceneDescriptorSetLayout, m_ShadowMapRefDescriptorSetLayout;
		std::vector<Ref<DescriptorSet>> m_CameraBufferDescriptorSets, m_SceneDataDescriptorSets, m_ShadowMapRefDescSets;
		std::vector<std::unique_ptr<Buffer>> m_CameraUniformBuffers, m_SceneUniformBuffers;
		Ref<Pipeline> m_MeshPipeline, m_SkymapPipeline, m_GridPipeline;
		VkSampler m_VkTextureSampler;
		Ref<Material> m_GridMaterial;
		TFrameResource<DescriptorSet> m_PostProcessingTargetImageDescSet;
		Ref<DescriptorSetLayout> m_PostProcessingTargetImageDescSetLayout;

		// Shadow Map
		Ref<RenderPass> m_ShadowMapRenderPass;
		Ref<Pipeline> m_ShadowMapPipeline;
		Ref<DescriptorSetLayout> m_ShadowMapDescriptorSetLayout;
		std::vector<Ref<DescriptorSet>> m_ShadowMapDescriptorSets;
		std::vector<std::unique_ptr<Buffer>> m_ShadowMapUniformBuffers;
		VkSampler m_ShadowMapSampler;

		Cascade m_Cascades[SceneRendererSettings::CascadeCount];
		SceneRendererSettings m_RendererSettings;

		// Jump Flood Algorithm
		Ref<Image> m_JumpFloodImage1[SwapChain::MAX_FRAMES_IN_FLIGHT], m_JumpFloodImage2[SwapChain::MAX_FRAMES_IN_FLIGHT];
		Ref<ComputePipeline> m_JumpFloodPipeline;
		Ref<DescriptorSet> m_JumpFloodDescSets[SwapChain::MAX_FRAMES_IN_FLIGHT];

		// Bloom Pass
		std::vector<TFrameResource<DescriptorSet>> m_BloomDescriptorSetResources;
		Ref<ComputePipeline> m_BloomPipeline;
		TFrameResource<Image> m_BloomImageResource;
		VkImageView m_BloomImageCompleteViews[SwapChain::MAX_FRAMES_IN_FLIGHT];
		VkSampler m_BloomSampler;

		// Compositing Pass
		Ref<RenderPass> m_CompositingPass;
		Ref<ComputePipeline> m_CompositingPipeline;

		// Textures
		Ref<Texture2D> m_PointLightIcon, m_SpotLightIcon, m_CameraIcon, m_DirectionalLightIcon;

		// Batching
		Unique<RendererData> m_RendererData;
	};

} // namespace Flameberry
