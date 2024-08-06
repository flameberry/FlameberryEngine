#pragma once

#include "CommandBuffer.h"
#include "DescriptorSet.h"
#include "Pipeline.h"
#include "MaterialAsset.h"
#include "ECS/Components.h"
#include "ECS/Scene.h"

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

		static const uint32_t CascadeCount = 4, CascadeSize = 1024 * 2; // TODO: Make this a renderer startup setting
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
		std::vector<RenderObject> RenderObjects;
	};

	///////////////////////////////////////////////////////////////////////////////////

	class SceneRenderer
	{
	public:
		SceneRenderer(const glm::vec2& viewportSize);
		~SceneRenderer();

		void RenderScene(const glm::vec2& viewportSize, const Ref<Scene>& scene, const GenericCamera& camera, const glm::vec3& cameraPosition, fbentt::entity selectedEntity, bool renderGrid = true, bool renderDebugIcons = true, bool renderOutline = true, bool renderPhysicsCollider = true);
		void RenderScene(const glm::vec2& viewportSize, const Ref<Scene>& scene, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec3& cameraPosition, float cameraNear, float cameraFar, fbentt::entity selectedEntity, bool renderGrid = true, bool renderDebugIcons = true, bool renderOutline = true, bool renderPhysicsCollider = true);

		VkImageView GetGeometryPassOutputImageView(uint32_t index) const { return m_GeometryPass->GetSpecification().TargetFramebuffers[index]->GetColorResolveAttachment(0)->GetVulkanImageView(); }
		VkImageView GetCompositePassOutputImageView(uint32_t index) const { return m_CompositePass->GetSpecification().TargetFramebuffers[index]->GetColorAttachment(0)->GetVulkanImageView(); }

		SceneRendererSettings& GetRendererSettingsRef() { return m_RendererSettings; }
		void RenderSceneForMousePicking(const Ref<Scene>& scene, const Ref<RenderPass>& renderPass, const Ref<Pipeline>& pipeline, const Ref<Pipeline>& pipeline2D, const glm::vec2& mousePos);

		void ReloadMeshShaders();

	private:
		void Init();

		void CalculateShadowMapCascades(const glm::mat4& viewProjectionMatrix, float cameraNear, float cameraFar, const glm::vec3& lightDirection);
		void SubmitPhysicsColliderGeometry(const Ref<Scene>& scene, fbentt::entity entity, TransformComponent& transform);
		void SubmitCameraViewGeometry(const Ref<Scene>& scene, fbentt::entity entity, TransformComponent& transform);

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

		// Shadow Map
		Ref<RenderPass> m_ShadowMapRenderPass;
		Ref<Pipeline> m_ShadowMapPipeline;
		Ref<DescriptorSetLayout> m_ShadowMapDescriptorSetLayout;
		std::vector<Ref<DescriptorSet>> m_ShadowMapDescriptorSets;
		std::vector<std::unique_ptr<Buffer>> m_ShadowMapUniformBuffers;
		VkSampler m_ShadowMapSampler;

		Cascade m_Cascades[SceneRendererSettings::CascadeCount];
		SceneRendererSettings m_RendererSettings;

		// Post processing
		Ref<RenderPass> m_CompositePass;
		Ref<Pipeline> m_CompositePipeline;
		Ref<DescriptorSetLayout> m_CompositePassDescriptorSetLayout;
		std::vector<Ref<DescriptorSet>> m_CompositePassDescriptorSets;

		// Textures
		Ref<Texture2D> m_PointLightIcon, m_SpotLightIcon, m_CameraIcon, m_DirectionalLightIcon;

		// Batching
		Unique<RendererData> m_RendererData;
	};

} // namespace Flameberry
