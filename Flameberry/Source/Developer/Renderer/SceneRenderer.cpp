#include "SceneRenderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Core/Core.h"
#include "Core/Log.h"
#include "Core/Profiler.h"

#include "ECS/Components.h"
#include "VulkanDebug.h"
#include "VulkanContext.h"
#include "Renderer.h"
#include "Renderer2D.h"
#include "RenderCommand.h"
#include "ShaderLibrary.h"
#include "Material.h"
#include "Frustum.h"
#include "Light.h"
#include "Skymap.h"

#include "Asset/AssetManager.h"

namespace Flameberry {

	struct CameraUniformBufferObject
	{
		glm::mat4 ViewMatrix, ProjectionMatrix, ViewProjectionMatrix;
	};

	struct SkymapPushConstantObject
	{
		glm::mat4 ViewProjectionMatrix;
		float Exposure;
	};

	struct SceneRendererSettingsUniform
	{
		int EnableShadows = 1, ShowCascades = 0, SoftShadows = 1, SkyReflections = 1;
		float GammaCorrectionFactor, Exposure;
	};

	struct SceneUniformBufferData
	{
		alignas(16) glm::mat4 CascadeViewProjectionMatrices[SceneRendererSettings::CascadeCount];
		alignas(16) float CascadeDepthSplits[SceneRendererSettings::CascadeCount];
		alignas(16) glm::vec3 cameraPosition;
		alignas(16) DirectionalLight directionalLight;
		alignas(16) PointLight PointLights[10];
		alignas(16) SpotLight SpotLights[10];
		alignas(4) int PointLightCount = 0, SpotLightCount = 0;
		alignas(4) float SkyLightIntensity = 0.0f;
		alignas(16) SceneRendererSettingsUniform RendererSettings;
	};

	SceneRenderer::SceneRenderer(const glm::vec2& viewportSize)
		: m_ViewportSize(viewportSize)
	{
		Init();
	}

	void SceneRenderer::Init()
	{
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		auto swapchain = VulkanContext::GetCurrentWindow()->GetSwapChain();
		auto imageCount = swapchain->GetSwapChainImageCount();
		auto sampleCount = RenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());
		auto swapchainImageFormat = swapchain->GetSwapChainImageFormat();

		m_RendererData = CreateUnique<RendererData>();

		/////////////////////////////////////// Preparing Shadow Mapping Pass ///////////////////////////////////////
		{
			FramebufferSpecification shadowMapFramebufferSpec;
			shadowMapFramebufferSpec.Width = SceneRendererSettings::CascadeSize;
			shadowMapFramebufferSpec.Height = SceneRendererSettings::CascadeSize;
			shadowMapFramebufferSpec.Attachments = { { VK_FORMAT_D32_SFLOAT, SceneRendererSettings::CascadeCount } };
			shadowMapFramebufferSpec.Samples = 1;
			shadowMapFramebufferSpec.DepthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			shadowMapFramebufferSpec.DepthStencilClearValue = { 1.0f, 0 };

			RenderPassSpecification shadowMapRenderPassSpec;
			shadowMapRenderPassSpec.TargetFramebuffers.resize(imageCount);
			shadowMapRenderPassSpec.Dependencies = {
				{
					VK_SUBPASS_EXTERNAL,						  // uint32_t                   srcSubpass
					0,											  // uint32_t                   dstSubpass
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,		  // VkPipelineStageFlags       srcStageMask
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,	  // VkPipelineStageFlags       dstStageMask
					VK_ACCESS_SHADER_READ_BIT,					  // VkAccessFlags              srcAccessMask
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, // VkAccessFlags              dstAccessMask
					VK_DEPENDENCY_BY_REGION_BIT					  // VkDependencyFlags          dependencyFlags
				},
				{
					0,											  // uint32_t                   srcSubpass
					VK_SUBPASS_EXTERNAL,						  // uint32_t                   dstSubpass
					VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,	  // VkPipelineStageFlags       srcStageMask
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,		  // VkPipelineStageFlags       dstStageMask
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, // VkAccessFlags              srcAccessMask
					VK_ACCESS_SHADER_READ_BIT,					  // VkAccessFlags              dstAccessMask
					VK_DEPENDENCY_BY_REGION_BIT					  // VkDependencyFlags          dependencyFlags
				}
			};

			for (uint32_t i = 0; i < imageCount; i++)
				shadowMapRenderPassSpec.TargetFramebuffers[i] = CreateRef<Framebuffer>(shadowMapFramebufferSpec);

			m_ShadowMapRenderPass = CreateRef<RenderPass>(shadowMapRenderPassSpec);

			// Shadow Map Pipeline
			auto bufferSize = sizeof(glm::mat4) * SceneRendererSettings::CascadeCount;

			BufferSpecification uniformBufferSpec;
			uniformBufferSpec.InstanceCount = 1;
			uniformBufferSpec.InstanceSize = bufferSize;
			uniformBufferSpec.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			uniformBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			m_ShadowMapUniformBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
			for (auto& uniformBuffer : m_ShadowMapUniformBuffers)
			{
				uniformBuffer = std::make_unique<Buffer>(uniformBufferSpec);
				uniformBuffer->MapMemory(bufferSize);
			}

			// Creating Descriptors
			DescriptorSetLayoutSpecification shadowDescSetLayoutSpec;
			shadowDescSetLayoutSpec.Bindings.emplace_back();
			shadowDescSetLayoutSpec.Bindings[0].binding = 0;
			shadowDescSetLayoutSpec.Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			shadowDescSetLayoutSpec.Bindings[0].descriptorCount = 1;
			shadowDescSetLayoutSpec.Bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			m_ShadowMapDescriptorSetLayout = DescriptorSetLayout::CreateOrGetCached(shadowDescSetLayoutSpec);

			DescriptorSetSpecification shadowMapDescSetSpec;
			shadowMapDescSetSpec.Layout = m_ShadowMapDescriptorSetLayout;

			m_ShadowMapDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
			for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
			{
				m_ShadowMapDescriptorSets[i] = CreateRef<DescriptorSet>(shadowMapDescSetSpec);

				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = m_ShadowMapUniformBuffers[i]->GetVulkanBuffer();
				bufferInfo.range = bufferSize;
				bufferInfo.offset = 0;

				m_ShadowMapDescriptorSets[i]->WriteBuffer(0, bufferInfo);

				m_ShadowMapDescriptorSets[i]->Update();
			}

			PipelineSpecification pipelineSpec{};
			pipelineSpec.Shader = ShaderLibrary::Get("DirectionalShadowMap");
			pipelineSpec.RenderPass = m_ShadowMapRenderPass;

			pipelineSpec.VertexLayout = {
				ShaderDataType::Float3,	 // a_Position
				ShaderDataType::Dummy12, // Normal (Unnecessary)
				ShaderDataType::Dummy8,	 // TextureCoords (Unnecessary)
				ShaderDataType::Dummy12, // Tangent (Unnecessary)
				ShaderDataType::Dummy12	 // BiTangent (Unnecessary)
			};

			pipelineSpec.Viewport.width = SceneRendererSettings::CascadeSize;
			pipelineSpec.Viewport.height = SceneRendererSettings::CascadeSize;
			pipelineSpec.Scissor = { { 0, 0 }, { SceneRendererSettings::CascadeSize, SceneRendererSettings::CascadeSize } };

			pipelineSpec.CullMode = VK_CULL_MODE_FRONT_BIT;
			pipelineSpec.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			pipelineSpec.DepthClampEnable = true;

			m_ShadowMapPipeline = CreateRef<Pipeline>(pipelineSpec);

			VkSamplerCreateInfo sampler_info{};
			sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			sampler_info.magFilter = VK_FILTER_LINEAR;
			sampler_info.minFilter = VK_FILTER_LINEAR;
			sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler_info.anisotropyEnable = VK_FALSE;
			sampler_info.mipLodBias = 0.0f;
			sampler_info.maxAnisotropy = 1.0f;
			sampler_info.minLod = 0.0f;
			sampler_info.maxLod = 1.0f;
			sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

			VK_CHECK_RESULT(vkCreateSampler(device, &sampler_info, nullptr, &m_ShadowMapSampler));
		}
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////// Preparing Geometry Pass //////////////////////////////////////////
		{
			// Scene
			VkDeviceSize uniformBufferSize = sizeof(CameraUniformBufferObject);

			BufferSpecification uniformBufferSpec;
			uniformBufferSpec.InstanceCount = 1;
			uniformBufferSpec.InstanceSize = uniformBufferSize;
			uniformBufferSpec.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			uniformBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			m_CameraUniformBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
			for (auto& uniformBuffer : m_CameraUniformBuffers)
			{
				uniformBuffer = std::make_unique<Buffer>(uniformBufferSpec);
				uniformBuffer->MapMemory(uniformBufferSize);
			}

			// Creating Descriptors
			DescriptorSetLayoutSpecification cameraBufferDescLayoutSpec;
			cameraBufferDescLayoutSpec.Bindings.emplace_back();

			cameraBufferDescLayoutSpec.Bindings[0].binding = 0;
			cameraBufferDescLayoutSpec.Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			cameraBufferDescLayoutSpec.Bindings[0].descriptorCount = 1;
			cameraBufferDescLayoutSpec.Bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			m_CameraBufferDescSetLayout = DescriptorSetLayout::CreateOrGetCached(cameraBufferDescLayoutSpec);

			DescriptorSetSpecification cameraBufferDescSetSpec;
			cameraBufferDescSetSpec.Layout = m_CameraBufferDescSetLayout;

			m_CameraBufferDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
			for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
			{
				m_CameraBufferDescriptorSets[i] = CreateRef<DescriptorSet>(cameraBufferDescSetSpec);

				VkDescriptorBufferInfo vk_descriptor_buffer_info{};
				vk_descriptor_buffer_info.buffer = m_CameraUniformBuffers[i]->GetVulkanBuffer();
				vk_descriptor_buffer_info.offset = 0;
				vk_descriptor_buffer_info.range = uniformBufferSize;

				m_CameraBufferDescriptorSets[i]->WriteBuffer(0, vk_descriptor_buffer_info);
				m_CameraBufferDescriptorSets[i]->Update();
			}

			FramebufferSpecification sceneFramebufferSpec;
			sceneFramebufferSpec.Width = m_ViewportSize.x;
			sceneFramebufferSpec.Height = m_ViewportSize.y;
			sceneFramebufferSpec.Attachments = { swapchainImageFormat, SwapChain::GetDepthFormat() };
			sceneFramebufferSpec.Samples = sampleCount;
			sceneFramebufferSpec.ClearColorValue = { 0.0f, 0.0f, 0.0f, 1.0f };
			sceneFramebufferSpec.DepthStencilClearValue = { 1.0f, 0 };
			// Don't store multisample color attachment
			sceneFramebufferSpec.ColorStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			// For outline compositing
			sceneFramebufferSpec.StencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

			RenderPassSpecification sceneRenderPassSpec;
			sceneRenderPassSpec.TargetFramebuffers.resize(imageCount);
			sceneRenderPassSpec.Dependencies = {
				{
					VK_SUBPASS_EXTERNAL,						  // uint32_t                   srcSubpass
					0,											  // uint32_t                   dstSubpass
					VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,	  // VkPipelineStageFlags       srcStageMask
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,		  // VkPipelineStageFlags       dstStageMask
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, // VkAccessFlags              srcAccessMask
					VK_ACCESS_SHADER_READ_BIT,					  // VkAccessFlags              dstAccessMask
					VK_DEPENDENCY_BY_REGION_BIT					  // VkDependencyFlags          dependencyFlags
				},
				{
					0,											   // uint32_t                   srcSubpass
					VK_SUBPASS_EXTERNAL,						   // uint32_t                   dstSubpass
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // VkPipelineStageFlags       srcStageMask
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,			   // VkPipelineStageFlags       dstStageMask
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,		   // VkAccessFlags              srcAccessMask
					VK_ACCESS_MEMORY_READ_BIT,					   // VkAccessFlags              dstAccessMask
					VK_DEPENDENCY_BY_REGION_BIT					   // VkDependencyFlags          dependencyFlags
				}
			};

			for (uint32_t i = 0; i < imageCount; i++)
				sceneRenderPassSpec.TargetFramebuffers[i] = CreateRef<Framebuffer>(sceneFramebufferSpec);

			m_GeometryPass = CreateRef<RenderPass>(sceneRenderPassSpec);

			{
				// Creating Mesh Pipeline
				VkDeviceSize uniformBufferSize = sizeof(SceneUniformBufferData);

				BufferSpecification bufferSpec;
				bufferSpec.InstanceCount = 1;
				bufferSpec.InstanceSize = uniformBufferSize;
				bufferSpec.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
				bufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

				m_SceneUniformBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
				for (auto& uniformBuffer : m_SceneUniformBuffers)
				{
					uniformBuffer = std::make_unique<Buffer>(bufferSpec);
					uniformBuffer->MapMemory(uniformBufferSize);
				}

				DescriptorSetLayoutSpecification sceneDescSetLayoutSpec;
				sceneDescSetLayoutSpec.Bindings.resize(1);

				// Scene Uniform Buffer
				sceneDescSetLayoutSpec.Bindings[0].binding = 0;
				sceneDescSetLayoutSpec.Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				sceneDescSetLayoutSpec.Bindings[0].descriptorCount = 1;
				sceneDescSetLayoutSpec.Bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

				m_SceneDescriptorSetLayout = DescriptorSetLayout::CreateOrGetCached(sceneDescSetLayoutSpec);

				DescriptorSetSpecification sceneDescSetSpec;
				sceneDescSetSpec.Layout = m_SceneDescriptorSetLayout;

				m_SceneDataDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
				for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
				{
					m_SceneDataDescriptorSets[i] = CreateRef<DescriptorSet>(sceneDescSetSpec);

					VkDescriptorBufferInfo bufferInfo{};
					bufferInfo.range = sizeof(SceneUniformBufferData);
					bufferInfo.offset = 0;
					bufferInfo.buffer = m_SceneUniformBuffers[i]->GetVulkanBuffer();

					m_SceneDataDescriptorSets[i]->WriteBuffer(0, bufferInfo);
					m_SceneDataDescriptorSets[i]->Update();
				}

				DescriptorSetLayoutSpecification shadowMapRefDescSetLayoutSpec;
				shadowMapRefDescSetLayoutSpec.Bindings.resize(1);

				// Shadow Map
				shadowMapRefDescSetLayoutSpec.Bindings[0].binding = 0;
				shadowMapRefDescSetLayoutSpec.Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				shadowMapRefDescSetLayoutSpec.Bindings[0].descriptorCount = 1;
				shadowMapRefDescSetLayoutSpec.Bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

				m_ShadowMapRefDescriptorSetLayout = DescriptorSetLayout::CreateOrGetCached(shadowMapRefDescSetLayoutSpec);

				DescriptorSetSpecification shadowMapRefDescSetSpec;
				shadowMapRefDescSetSpec.Layout = m_ShadowMapRefDescriptorSetLayout;

				m_ShadowMapRefDescSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
				for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
				{
					m_ShadowMapRefDescSets[i] = CreateRef<DescriptorSet>(shadowMapRefDescSetSpec);

					VkDescriptorImageInfo imageInfo{};
					imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
					imageInfo.imageView = m_ShadowMapRenderPass->GetSpecification().TargetFramebuffers[i]->GetDepthAttachment()->GetVulkanImageView();
					imageInfo.sampler = m_ShadowMapSampler;

					m_ShadowMapRefDescSets[i]->WriteImage(0, imageInfo);
					m_ShadowMapRefDescSets[i]->Update();
				}

				PipelineSpecification pipelineSpec{};

				pipelineSpec.Shader = ShaderLibrary::Get("PBR");
				pipelineSpec.RenderPass = m_GeometryPass;

				pipelineSpec.VertexLayout = {
					ShaderDataType::Float3, // a_Position
					ShaderDataType::Float3, // a_Normal
					ShaderDataType::Float2, // a_TextureCoords
					ShaderDataType::Float3, // a_Tangent
					ShaderDataType::Float3	// a_BiTangent
				};
				pipelineSpec.Samples = RenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());

				pipelineSpec.BlendingEnable = true;

				pipelineSpec.StencilTestEnable = true;
				pipelineSpec.StencilOpState.failOp = VK_STENCIL_OP_KEEP;
				pipelineSpec.StencilOpState.depthFailOp = VK_STENCIL_OP_REPLACE;
				pipelineSpec.StencilOpState.passOp = VK_STENCIL_OP_REPLACE;
				pipelineSpec.StencilOpState.compareOp = VK_COMPARE_OP_ALWAYS;
				pipelineSpec.StencilOpState.compareMask = 0xFF;
				pipelineSpec.StencilOpState.reference = 1;
				pipelineSpec.StencilOpState.writeMask = 1;

				m_MeshPipeline = CreateRef<Pipeline>(pipelineSpec);
			}

			// Skybox Pipeline
			{
				Flameberry::PipelineSpecification pipelineSpec{};
				pipelineSpec.Shader = ShaderLibrary::Get("SkyMap");
				pipelineSpec.RenderPass = m_GeometryPass;

				pipelineSpec.VertexLayout = {};

				VkSampleCountFlagBits sampleCount = RenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());
				pipelineSpec.Samples = sampleCount;

				pipelineSpec.CullMode = VK_CULL_MODE_NONE;

				pipelineSpec.DepthTestEnable = true;
				pipelineSpec.DepthWriteEnable = false;
				pipelineSpec.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

				m_SkymapPipeline = CreateRef<Pipeline>(pipelineSpec);
			}
			m_VkTextureSampler = Texture2D::GetDefaultSampler();
		}
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
        ////////////////////////////////////////// Preparing Composite Pass /////////////////////////////////////////
        {
            // Create render pass
            FramebufferSpecification framebufferSpec{};
            framebufferSpec.Width = m_ViewportSize.x;
            framebufferSpec.Height = m_ViewportSize.y;
            framebufferSpec.Attachments = { swapchainImageFormat, VK_FORMAT_D32_SFLOAT };
            framebufferSpec.ClearColorValue = { 0.0f, 0.0f, 0.0f, 1.0f };
            framebufferSpec.DepthStencilClearValue = { 1.0f, 0 };
            framebufferSpec.Samples = 1;

            RenderPassSpecification renderPassSpec{};
            renderPassSpec.TargetFramebuffers.resize(swapchain->GetSwapChainImageCount());
            for (uint32_t i = 0; i < renderPassSpec.TargetFramebuffers.size(); i++)
                renderPassSpec.TargetFramebuffers[i] = CreateRef<Framebuffer>(framebufferSpec);

            m_CompositePass = CreateRef<RenderPass>(renderPassSpec);

            DescriptorSetLayoutSpecification layoutSpec;
            layoutSpec.Bindings.resize(1);

            layoutSpec.Bindings[0].binding = 0;
            layoutSpec.Bindings[0].descriptorCount = 1;
            layoutSpec.Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            layoutSpec.Bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            m_CompositePassDescriptorSetLayout = DescriptorSetLayout::CreateOrGetCached(layoutSpec);

            DescriptorSetSpecification setSpec;
            setSpec.Layout = m_CompositePassDescriptorSetLayout;

            m_CompositePassDescriptorSets.resize(imageCount);
            for (uint8_t i = 0; i < m_CompositePassDescriptorSets.size(); i++)
            {
                m_CompositePassDescriptorSets[i] = CreateRef<DescriptorSet>(setSpec);

                VkDescriptorImageInfo imageInfo{
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = m_GeometryPass->GetSpecification().TargetFramebuffers[i]->GetColorResolveAttachment(0)->GetImageView(),
                    .sampler = m_VkTextureSampler
                };

                m_CompositePassDescriptorSets[i]->WriteImage(0, imageInfo);

                m_CompositePassDescriptorSets[i]->Update();
            }

            PipelineSpecification pipelineSpec{};
            pipelineSpec.PipelineLayout.DescriptorSetLayouts = { m_CompositePassDescriptorSetLayout };

            pipelineSpec.Shader = CreateRef<Shader>(FBY_PROJECT_DIR"Flameberry/shaders/vulkan/bin/composite.vert.spv", FBY_PROJECT_DIR"Flameberry/shaders/vulkan/bin/composite.frag.spv");
            pipelineSpec.RenderPass = m_CompositePass;

            pipelineSpec.VertexLayout = {};

            pipelineSpec.BlendingEnable = true;
            pipelineSpec.CullMode = VK_CULL_MODE_FRONT_BIT;

            m_CompositePipeline = CreateRef<Pipeline>(pipelineSpec);
        }
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

		Renderer2D::Init(m_GeometryPass);

		// Textures
		m_PointLightIcon = Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR "Flameberry/Assets/Icons/BulbIcon.png");
		m_SpotLightIcon = Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR "Flameberry/Assets/Icons/SpotLightIcon.png");
		m_CameraIcon = Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR "Flameberry/Assets/Icons/CameraIcon.png");
		m_DirectionalLightIcon = Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR "Flameberry/Assets/Icons/SunIcon.png");
	}

	void SceneRenderer::RenderScene(const glm::vec2& viewportSize, const Ref<Scene>& scene, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec3& cameraPosition, float cameraNear, float cameraFar, fbentt::entity selectedEntity, bool renderGrid, bool renderDebugIcons, bool renderOutline, bool renderPhysicsCollider)
	{
		uint32_t currentFrame = Renderer::GetCurrentFrameIndex();
		std::vector<fbentt::entity> pointLightEntityHandles, spotLightEntityHandles;

		m_ViewportSize = viewportSize;

		// Resize Framebuffers
		Renderer::Submit([&](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
			const auto& framebufferSpec = m_GeometryPass->GetSpecification().TargetFramebuffers[imageIndex]->GetSpecification();
			if (!(m_ViewportSize.x == 0 || m_ViewportSize.y == 0) && (framebufferSpec.Width != m_ViewportSize.x || framebufferSpec.Height != m_ViewportSize.y))
			{
				m_GeometryPass->GetSpecification().TargetFramebuffers[imageIndex]->OnResize(m_ViewportSize.x, m_ViewportSize.y, m_GeometryPass->GetRenderPass());

#if 0
                    VkDescriptorImageInfo imageInfo{
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        .imageView = m_GeometryPass->GetSpecification().TargetFramebuffers[imageIndex]->GetColorResolveAttachment(0)->GetImageView(),
                        .sampler = m_VkTextureSampler
                    };

                    m_CompositePassDescriptorSets[imageIndex]->WriteImage(0, imageInfo);

                    m_CompositePassDescriptorSets[imageIndex]->Update();
                    m_CompositePass->GetSpecification().TargetFramebuffers[imageIndex]->OnResize(m_ViewportSize.x, m_ViewportSize.y, m_CompositePass->GetRenderPass());
#endif
			}

			// VkClearColorValue color = { scene->GetClearColor().x, scene->GetClearColor().y, scene->GetClearColor().z, 1.0f };
			// m_GeometryPass->GetSpecification().TargetFramebuffers[imageIndex]->SetClearColorValue(color);
		});

		// Update uniform buffers
		CameraUniformBufferObject cameraBufferData;
		cameraBufferData.ViewMatrix = viewMatrix;
		cameraBufferData.ProjectionMatrix = projectionMatrix;
		cameraBufferData.ViewProjectionMatrix = projectionMatrix * viewMatrix;

		m_CameraUniformBuffers[currentFrame]->WriteToBuffer(&cameraBufferData, sizeof(CameraUniformBufferObject));

		SceneUniformBufferData sceneUniformBufferData;

		// Keep the skylight ready
		SkyLightComponent* skyMap = nullptr;
		for (const auto entity : scene->m_Registry->group<TransformComponent, SkyLightComponent>())
		{
			skyMap = scene->m_Registry->try_get<SkyLightComponent>(entity);
			sceneUniformBufferData.SkyLightIntensity = skyMap->Intensity;
		}

		// Important variable
		bool shouldRenderShadows = false;

		// Update Directional Lights
		for (const auto& entity : scene->m_Registry->group<TransformComponent, DirectionalLightComponent>())
		{
			auto [transform, dirLight] = scene->m_Registry->get<TransformComponent, DirectionalLightComponent>(entity);
			sceneUniformBufferData.directionalLight.Color = dirLight.Color;
			sceneUniformBufferData.directionalLight.Intensity = dirLight.Intensity;

			// NOTE: X direction is 0.000001f to avoid shadows being not rendered when directional light perspective camera is looking directly downwards
			sceneUniformBufferData.directionalLight.Direction = glm::rotate(glm::quat(transform.Rotation), glm::vec3(0.000001f, -1.0f, 0.0f));
			sceneUniformBufferData.directionalLight.LightSize = dirLight.LightSize;

			shouldRenderShadows = m_RendererSettings.EnableShadows && true;
		}

		if (shouldRenderShadows)
		{
			// TODO: Calculate these only when camera or directional light is updated
			CalculateShadowMapCascades(cameraBufferData.ViewProjectionMatrix, cameraNear, cameraFar, sceneUniformBufferData.directionalLight.Direction);
			glm::mat4 cascades[SceneRendererSettings::CascadeCount];
			for (uint8_t i = 0; i < SceneRendererSettings::CascadeCount; i++)
				cascades[i] = m_Cascades[i].ViewProjectionMatrix;

			m_ShadowMapUniformBuffers[currentFrame]->WriteToBuffer(cascades, sizeof(glm::mat4) * SceneRendererSettings::CascadeCount);
		}

		sceneUniformBufferData.cameraPosition = cameraPosition;

		for (uint32_t i = 0; i < SceneRendererSettings::CascadeCount; i++)
		{
			sceneUniformBufferData.CascadeViewProjectionMatrices[i] = m_Cascades[i].ViewProjectionMatrix;
			sceneUniformBufferData.CascadeDepthSplits[i] = m_Cascades[i].DepthSplit;
		}
		sceneUniformBufferData.RendererSettings.EnableShadows = (int)shouldRenderShadows;
		sceneUniformBufferData.RendererSettings.ShowCascades = (int)m_RendererSettings.ShowCascades;
		sceneUniformBufferData.RendererSettings.SoftShadows = (int)m_RendererSettings.SoftShadows;
		sceneUniformBufferData.RendererSettings.SkyReflections = (int)m_RendererSettings.SkyReflections;
		sceneUniformBufferData.RendererSettings.GammaCorrectionFactor = m_RendererSettings.GammaCorrectionFactor;
		sceneUniformBufferData.RendererSettings.Exposure = m_RendererSettings.Exposure;

		for (const auto& entity : scene->m_Registry->group<TransformComponent, PointLightComponent>())
		{
			const auto& [transform, light] = scene->m_Registry->get<TransformComponent, PointLightComponent>(entity);
			sceneUniformBufferData.PointLights[sceneUniformBufferData.PointLightCount].Position = transform.Translation;
			sceneUniformBufferData.PointLights[sceneUniformBufferData.PointLightCount].Color = light.Color;
			sceneUniformBufferData.PointLights[sceneUniformBufferData.PointLightCount].Intensity = light.Intensity;
			sceneUniformBufferData.PointLightCount++;

			pointLightEntityHandles.emplace_back(entity);
		}

		for (const auto& entity : scene->m_Registry->group<TransformComponent, SpotLightComponent>())
		{
			const auto& [transform, light] = scene->m_Registry->get<TransformComponent, SpotLightComponent>(entity);
			sceneUniformBufferData.SpotLights[sceneUniformBufferData.SpotLightCount].Position = transform.Translation;
			sceneUniformBufferData.SpotLights[sceneUniformBufferData.SpotLightCount].Direction = glm::rotate(glm::quat(transform.Rotation), glm::vec3(0.000001f, -1.0f, 0.0f));
			sceneUniformBufferData.SpotLights[sceneUniformBufferData.SpotLightCount].Color = light.Color;
			sceneUniformBufferData.SpotLights[sceneUniformBufferData.SpotLightCount].Intensity = light.Intensity;
			sceneUniformBufferData.SpotLights[sceneUniformBufferData.SpotLightCount].InnerConeAngle = glm::radians(light.InnerConeAngle);
			sceneUniformBufferData.SpotLights[sceneUniformBufferData.SpotLightCount].OuterConeAngle = glm::radians(light.OuterConeAngle);
			sceneUniformBufferData.SpotLightCount++;

			spotLightEntityHandles.emplace_back(entity);
		}

		m_SceneUniformBuffers[currentFrame]->WriteToBuffer(&sceneUniformBufferData, sizeof(SceneUniformBufferData));

		/////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////// Shadow Mapping Pass ///////////////////////////////////////////

		if (shouldRenderShadows)
		{
			m_ShadowMapRenderPass->Begin();
			Renderer::Submit([shadowMapDescSet = m_ShadowMapDescriptorSets[currentFrame]->GetVulkanDescriptorSet(), shadowMapPipelineLayout = m_ShadowMapPipeline->GetVulkanPipelineLayout(), pipeline = m_ShadowMapPipeline->GetVulkanPipeline()](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
				// Binding the shadow map pipeline here instead of using the `Pipeline::Bind()` function to reduce `Renderer::Submit()` calls
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipelineLayout, 0, 1, &shadowMapDescSet, 0, nullptr);
			});

			for (const auto& entity : scene->m_Registry->group<TransformComponent, MeshComponent>())
			{
				const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

				if (auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle))
				{
					ModelMatrixPushConstantData pushContantData;
					pushContantData.ModelMatrix = transform.CalculateTransform();
					Renderer::Submit([staticMesh, shadowMapPipelineLayout = m_ShadowMapPipeline->GetVulkanPipelineLayout(), pushContantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
						vkCmdPushConstants(cmdBuffer, shadowMapPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrixPushConstantData), &pushContantData);
						Renderer::RT_BindVertexAndIndexBuffers(cmdBuffer, staticMesh->GetVertexBuffer()->GetVulkanBuffer(), staticMesh->GetIndexBuffer()->GetVulkanBuffer());
						const uint32_t size = staticMesh->GetSubMeshes().back().IndexOffset + staticMesh->GetSubMeshes().back().IndexCount;
						vkCmdDrawIndexed(cmdBuffer, size, 1, 0, 0, 0);
					});
				}
			}
			m_ShadowMapRenderPass->End();
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////// Geometry Pass //////////////////////////////////////////////

		m_GeometryPass->Begin();

		RenderCommand::SetViewport(0.0f, 0.0f, m_ViewportSize.x, m_ViewportSize.y);
		RenderCommand::SetScissor({ 0, 0 }, VkExtent2D{ (uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y });

		/////////////////////////////////////////// SkyMap Rendering ////////////////////////////////////////////

		bool shouldRenderSkyMap = skyMap && skyMap->EnableSkyMap && skyMap->SkyMap;
		VkDescriptorSet textureDescSet = VK_NULL_HANDLE;

		if (shouldRenderSkyMap)
		{
			VkPipelineLayout pipelineLayout = m_SkymapPipeline->GetVulkanPipelineLayout();
			textureDescSet = AssetManager::GetAsset<Skymap>(skyMap->SkyMap)->GetDescriptorSet()->GetVulkanDescriptorSet();

			SkymapPushConstantObject pco;
			pco.ViewProjectionMatrix = projectionMatrix * glm::mat4(glm::mat3(viewMatrix));
			pco.Exposure = m_RendererSettings.Exposure;

			Renderer::Submit([pipeline = m_SkymapPipeline->GetVulkanPipeline(), pipelineLayout, pco, textureDescSet](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
				Renderer::RT_BindPipeline(cmdBuffer, pipeline);
				VkDescriptorSet descSets[] = { textureDescSet };
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, descSets, 0, nullptr);
				vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkymapPushConstantObject), &pco);
				vkCmdDraw(cmdBuffer, 36, 1, 0, 0);
			});
		}

		///////////////////////////////////////// Mesh Pipeline Binding //////////////////////////////////////////

		Renderer::Submit([=, pipeline = m_MeshPipeline->GetVulkanPipeline(), pipelineLayout = m_MeshPipeline->GetVulkanPipelineLayout()](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
			VkDescriptorSet descriptorSets[] = {
				m_CameraBufferDescriptorSets[currentFrame]->GetVulkanDescriptorSet(),
				m_SceneDataDescriptorSets[currentFrame]->GetVulkanDescriptorSet(),
				m_ShadowMapRefDescSets[imageIndex]->GetVulkanDescriptorSet(),
				shouldRenderSkyMap ? textureDescSet : Skymap::GetEmptyDescriptorSet()->GetVulkanDescriptorSet()
			};

			Renderer::RT_BindPipeline(cmdBuffer, pipeline);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, sizeof(descriptorSets) / sizeof(VkDescriptorSet), descriptorSets, 0, nullptr);
		});

#if 0
        // Without sorting
        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);
            if (auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle); staticMesh)
                Renderer::SubmitMeshWithMaterial(staticMesh, m_MeshPipeline, mesh.OverridenMaterialTable, transform.GetTransform());
        }
#else
		// With sorting

		// TODO: Temporarily placing this code here, it is inefficient to keep this array filled till the next frame
		m_RendererData->RenderObjects.clear();

		/////////////////////////////////////// Gathering All Render Objects ///////////////////////////////////////

		// TODO: Move this someplace better
		Frustum cameraFrustum;

		if (m_RendererSettings.FrustumCulling)
			cameraFrustum.ExtractFrustumPlanes(cameraBufferData.ViewProjectionMatrix);

		for (const auto& entity : scene->m_Registry->group<TransformComponent, MeshComponent>())
		{
			const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

			if (auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle))
			{
				uint32_t submeshIndex = 0;

				m_RendererData->RenderObjects.reserve(m_RendererData->RenderObjects.size() + staticMesh->GetSubMeshes().size());

				for (const auto& submesh : staticMesh->GetSubMeshes())
				{
					if (m_RendererSettings.FrustumCulling)
					{
						const auto modelMatrix = transform.CalculateTransform();

						// TODO: Move this outside of the `if (m_RendererSettings.FrustumCulling)`
						if (m_RendererSettings.ShowBoundingBoxes)
							Renderer2D::AddAABB(submesh.AABB, modelMatrix, glm::vec4(1, 1, 0, 1));

						// Skip processing the mesh if it is out of the camera frustum
						if (!IsAABBInsideFrustum(submesh.AABB, modelMatrix, cameraFrustum))
							continue;
					}

					// The Assumption here is every mesh loaded will have a Material, i.e. materialAsset won't be nullptr
					Ref<MaterialAsset> materialAsset;
					if (const auto it = mesh.OverridenMaterialTable.find(submeshIndex); it != mesh.OverridenMaterialTable.end())
						materialAsset = AssetManager::GetAsset<MaterialAsset>(it->second);
					else if (AssetManager::IsAssetHandleValid(submesh.MaterialHandle))
						materialAsset = AssetManager::GetAsset<MaterialAsset>(submesh.MaterialHandle);

					// Add it to final list of render objects
					m_RendererData->RenderObjects.emplace_back(
						staticMesh->GetVertexBuffer()->GetVulkanBuffer(),
						staticMesh->GetIndexBuffer()->GetVulkanBuffer(),
						submesh.IndexOffset,
						submesh.IndexCount,
						&transform,
						materialAsset);

					submeshIndex++;
				}
			}
		}

		///////////////////////////////////////////////// Sorting /////////////////////////////////////////////////
		{
			FBY_PROFILE_SCOPE("Sort_RenderObjects");
			/// Sorting the render objects according to the material IDs
			auto cmp = [](const RenderObject& a, const RenderObject& b) { return a.MaterialAsset->Handle < b.MaterialAsset->Handle; };
			std::stable_sort(m_RendererData->RenderObjects.begin(), m_RendererData->RenderObjects.end(), cmp);
		}

		//////////////////////////////////////////////// Rendering ////////////////////////////////////////////////

		AssetHandle boundMaterialHandle = 0;
		VkBuffer boundVertexBuffer = VK_NULL_HANDLE;
		TransformComponent* boundTransform = nullptr;

		for (const auto& obj : m_RendererData->RenderObjects)
		{
			Renderer::Submit([bindMaterial = boundMaterialHandle != obj.MaterialAsset->Handle,
								 bindVertexAndIndexBuffers = boundVertexBuffer != obj.VertexBuffer,
								 bindTransform = boundTransform != obj.Transform,
								 pipelineLayout = m_MeshPipeline->GetVulkanPipelineLayout(),
								 material = obj.MaterialAsset->GetUnderlyingMaterial(),
								 vertexBuffer = obj.VertexBuffer,
								 indexBuffer = obj.IndexBuffer,
								 transform = obj.Transform->CalculateTransform(),
								 indexCount = obj.IndexCount,
								 indexOffset = obj.IndexOffset](VkCommandBuffer cmdBuffer, uint32_t) {
				if (bindMaterial)
					Renderer::RT_BindMaterial(cmdBuffer, pipelineLayout, material);

				if (bindVertexAndIndexBuffers)
					Renderer::RT_BindVertexAndIndexBuffers(cmdBuffer, vertexBuffer, indexBuffer);

				if (bindTransform)
					vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(transform), glm::value_ptr(transform));

				// Draw the object
				vkCmdDrawIndexed(cmdBuffer, indexCount, 1, indexOffset, 0, 0);
			});

			boundMaterialHandle = obj.MaterialAsset->Handle;
			boundVertexBuffer = obj.VertexBuffer;
			boundTransform = obj.Transform;
		}
#endif
		////////////////////////////////////////////// 2D Rendering //////////////////////////////////////////////

		Renderer2D::BeginScene(m_CameraBufferDescriptorSets[currentFrame]->GetVulkanDescriptorSet());

		if (renderDebugIcons)
		{
			// Render Point Light Icons
			Renderer2D::SetActiveTexture(m_PointLightIcon);
			for (uint32_t i = 0; i < sceneUniformBufferData.PointLightCount; i++)
				Renderer2D::AddBillboard(sceneUniformBufferData.PointLights[i].Position, 0.7f, sceneUniformBufferData.PointLights[i].Color, viewMatrix, fbentt::to_index(pointLightEntityHandles[i]));
			Renderer2D::FlushQuads();

			// Render Spot Light Icons
			Renderer2D::SetActiveTexture(m_SpotLightIcon);
			for (uint32_t i = 0; i < sceneUniformBufferData.SpotLightCount; i++)
				Renderer2D::AddBillboard(sceneUniformBufferData.SpotLights[i].Position, 1.0f, sceneUniformBufferData.SpotLights[i].Color, viewMatrix, fbentt::to_index(spotLightEntityHandles[i]));
			Renderer2D::FlushQuads();

			Renderer2D::SetActiveTexture(m_CameraIcon);
			for (auto entity : scene->m_Registry->group<TransformComponent, CameraComponent>())
			{
				auto& transform = scene->m_Registry->get<TransformComponent>(entity);
				Renderer2D::AddBillboard(transform.Translation, 0.7f, glm::vec3(1), viewMatrix, fbentt::to_index(entity));
			}
			Renderer2D::FlushQuads();

			Renderer2D::SetActiveTexture(m_DirectionalLightIcon);
			for (auto entity : scene->m_Registry->group<TransformComponent, DirectionalLightComponent>())
			{
				auto& transform = scene->m_Registry->get<TransformComponent>(entity);
				Renderer2D::AddBillboard(transform.Translation, 1.2f, glm::vec3(1), viewMatrix, fbentt::to_index(entity));
			}
			Renderer2D::FlushQuads();
		}

		if (renderPhysicsCollider && selectedEntity != fbentt::null)
		{
			auto& transform = scene->m_Registry->get<TransformComponent>(selectedEntity);

			// Draw Physics Collider
			SubmitPhysicsColliderGeometry(scene, selectedEntity, transform); // NOTE: This function will check if any of the colliders is present
			SubmitCameraViewGeometry(scene, selectedEntity, transform);
		}

		// Should make editor only (Not Runtime)
		if (renderGrid)
			Renderer2D::AddGrid(25);

		// Render all the text in the scene
		for (const auto entity : scene->m_Registry->group<TransformComponent, TextComponent>())
		{
			const auto& [transform, text] = scene->m_Registry->get<TransformComponent, TextComponent>(entity);
			Renderer2D::AddText(text.TextString, text.Font, transform.CalculateTransform(), { text.Color, text.Kerning, text.LineSpacing }, fbentt::to_index(entity));
		}

		Renderer2D::EndScene();

		m_GeometryPass->End();

#if 0
        /////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////// Composite Pass /////////////////////////////////////////////

        m_CompositePass->Begin();
        m_CompositePipeline->Bind();

        std::vector<VkDescriptorSet> descSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
        for (uint8_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
            descSets[i] = m_CompositePassDescriptorSets[i]->GetDescriptorSet();

        Renderer::Submit([pipelineLayout = m_CompositePipeline->GetLayout(), descSets](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSets[imageIndex], 0, nullptr);
                vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
            }
        );
        m_CompositePass->End();

        /////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
	}

	void SceneRenderer::CalculateShadowMapCascades(const glm::mat4& viewProjectionMatrix, float cameraNear, float cameraFar, const glm::vec3& lightDirection)
	{
		float cascadeSplits[SceneRendererSettings::CascadeCount];

		const float nearClip = cameraNear;
		const float farClip = cameraFar;
		const float clipRange = farClip - nearClip;

		const float minZ = nearClip;
		const float maxZ = nearClip + clipRange;

		const float range = maxZ - minZ;
		const float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SceneRendererSettings::CascadeCount; i++)
		{
			const float p = (i + 1) / static_cast<float>(SceneRendererSettings::CascadeCount);
			const float log = minZ * std::pow(ratio, p);
			const float uniform = minZ + range * p;
			const float d = m_RendererSettings.CascadeLambdaSplit * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;

		// Project frustum corners into world space
		const glm::mat4 invCam = glm::inverse(viewProjectionMatrix);

		for (uint32_t i = 0; i < SceneRendererSettings::CascadeCount; i++)
		{
			const float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] = {
				glm::vec3(-1.0f, 1.0f, 0.0f),
				glm::vec3(1.0f, 1.0f, 0.0f),
				glm::vec3(1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f, 1.0f, 1.0f),
				glm::vec3(1.0f, 1.0f, 1.0f),
				glm::vec3(1.0f, -1.0f, 1.0f),
				glm::vec3(-1.0f, -1.0f, 1.0f)
			};

			for (uint32_t i = 0; i < 8; i++)
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++)
			{
				frustumCenter += frustumCorners[i];
			}
			frustumCenter /= 8.0f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			// radius = std::ceil(radius * 16.0f) / 16.0f;

			const float width = frustumCorners[5].x - frustumCorners[4].x;
			const float height = frustumCorners[7].y - frustumCorners[5].y;

			// const float fWorldUnitsPerTexel = 2.0f * radius * 0.70710678118f / (float)SceneRendererSettings::CascadeSize;
			// const float fInvWorldUnitsPerTexel = 1.0f / fWorldUnitsPerTexel;

			const glm::vec3 vWorldUnitsPerTexel(width / (float)SceneRendererSettings::CascadeSize, height / (float)SceneRendererSettings::CascadeSize, 1.0f);
			const glm::vec3 vInvWorldUnitsPerTexel = 1.0f / vWorldUnitsPerTexel;

			glm::vec3 maxExtents = (glm::floor(glm::vec3(radius)) * vInvWorldUnitsPerTexel) * vWorldUnitsPerTexel;
			frustumCenter = (glm::floor(frustumCenter) * vInvWorldUnitsPerTexel) * vWorldUnitsPerTexel;
			glm::vec3 minExtents = -maxExtents;

			const glm::vec3 lightDir = glm::normalize(lightDirection);
			const glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter + lightDir * minExtents, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
			const glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

			// Store split distance and matrix in cascade
			m_Cascades[i].DepthSplit = (cameraNear + splitDist * clipRange) * -1.0f;
			m_Cascades[i].ViewProjectionMatrix = lightOrthoMatrix * lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}

	void SceneRenderer::RenderSceneForMousePicking(const Ref<Scene>& scene, const Ref<RenderPass>& renderPass, const Ref<Pipeline>& pipeline, const Ref<Pipeline>& pipeline2D, const glm::vec2& mousePos)
	{
		renderPass->Begin(0, { (int)mousePos.x, (int)mousePos.y }, { 1, 1 });

		Renderer::Submit([pipeline = pipeline->GetVulkanPipeline(), descSet = m_CameraBufferDescriptorSets[Renderer::GetCurrentFrameIndex()]->GetVulkanDescriptorSet(), mousePickingPipelineLayout = pipeline->GetVulkanPipelineLayout()](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
			Renderer::RT_BindPipeline(cmdBuffer, pipeline);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mousePickingPipelineLayout, 0, 1, &descSet, 0, nullptr);
		});

		for (const auto& entity : scene->m_Registry->group<TransformComponent, MeshComponent>())
		{
			const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

			if (auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle))
			{
				MousePickingPushConstantData pushContantData;
				pushContantData.ModelMatrix = transform.CalculateTransform();
				pushContantData.EntityIndex = fbentt::to_index(entity);

				Renderer::Submit([staticMesh, mousePickingPipelineLayout = pipeline->GetVulkanPipelineLayout(), pushContantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
					vkCmdPushConstants(cmdBuffer, mousePickingPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MousePickingPushConstantData), &pushContantData);
					Renderer::RT_BindVertexAndIndexBuffers(cmdBuffer, staticMesh->GetVertexBuffer()->GetVulkanBuffer(), staticMesh->GetIndexBuffer()->GetVulkanBuffer());

					const uint32_t size = staticMesh->GetSubMeshes().back().IndexOffset + staticMesh->GetSubMeshes().back().IndexCount;
					vkCmdDrawIndexed(cmdBuffer, size, 1, 0, 0, 0);
				});
			}
		}

		// 2D Quad Entities
		uint32_t indexCount = 6 * Renderer2D::GetRendererData().QuadVertexBufferOffset / (4 * sizeof(QuadVertex));
		Renderer::Submit([descSet = m_CameraBufferDescriptorSets[Renderer::GetCurrentFrameIndex()]->GetVulkanDescriptorSet(),
							 mousePicking2DPipelineLayout = pipeline2D->GetVulkanPipelineLayout(),
							 vulkanPipeline2D = pipeline2D->GetVulkanPipeline(),
							 vertexBuffer = Renderer2D::GetRendererData().QuadVertexBuffer->GetVulkanBuffer(),
							 indexBuffer = Renderer2D::GetRendererData().QuadIndexBuffer->GetVulkanBuffer(),
							 indexCount](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
			Renderer::RT_BindPipeline(cmdBuffer, vulkanPipeline2D);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mousePicking2DPipelineLayout, 0, 1, &descSet, 0, nullptr);

			Renderer::RT_BindVertexAndIndexBuffers(cmdBuffer, vertexBuffer, indexBuffer);
			vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);
		});

		// Text Entities
		indexCount = 6 * Renderer2D::GetRendererData().TextVertexBufferOffset / (4 * sizeof(TextVertex));
		Renderer::Submit([descSet = m_CameraBufferDescriptorSets[Renderer::GetCurrentFrameIndex()]->GetVulkanDescriptorSet(),
							 mousePicking2DPipelineLayout = pipeline2D->GetVulkanPipelineLayout(),
							 vulkanPipeline2D = pipeline2D->GetVulkanPipeline(),
							 vertexBuffer = Renderer2D::GetRendererData().TextVertexBuffer->GetVulkanBuffer(),
							 indexBuffer = Renderer2D::GetRendererData().TextIndexBuffer->GetVulkanBuffer(),
							 indexCount](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
			Renderer::RT_BindVertexAndIndexBuffers(cmdBuffer, vertexBuffer, indexBuffer);
			vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);
		});

		renderPass->End();
	}

	void SceneRenderer::ReloadMeshShaders()
	{
		m_MeshPipeline->ReloadShaders();
	}

	// TODO: Move this to EditorLayer.cpp ASAP
	void SceneRenderer::SubmitPhysicsColliderGeometry(const Ref<Scene>& scene, fbentt::entity entity, TransformComponent& transform)
	{
		// TODO: Optimise this function (maybe embed the vertices (?))
		constexpr glm::vec3 greenColor(0.2f, 1.0f, 0.2f);
		constexpr float bias(0.001f);
		const glm::mat3 rotationMatrix = glm::toMat3(glm::quat(transform.Rotation));

		// Render Physics Colliders
		auto* boxCollider = scene->m_Registry->try_get<BoxColliderComponent>(entity);
		if (boxCollider)
		{
			const glm::vec3 halfExtent = transform.Scale * boxCollider->Size * 0.5f + bias;

			// Calculate the positions of the vertices of the collider
			glm::vec3 vertex1 = glm::vec3(transform.Translation + rotationMatrix * glm::vec3(-halfExtent.x, -halfExtent.y, -halfExtent.z));
			glm::vec3 vertex2 = glm::vec3(transform.Translation + rotationMatrix * glm::vec3(halfExtent.x, -halfExtent.y, -halfExtent.z));
			glm::vec3 vertex3 = glm::vec3(transform.Translation + rotationMatrix * glm::vec3(halfExtent.x, halfExtent.y, -halfExtent.z));
			glm::vec3 vertex4 = glm::vec3(transform.Translation + rotationMatrix * glm::vec3(-halfExtent.x, halfExtent.y, -halfExtent.z));
			glm::vec3 vertex5 = glm::vec3(transform.Translation + rotationMatrix * glm::vec3(-halfExtent.x, -halfExtent.y, halfExtent.z));
			glm::vec3 vertex6 = glm::vec3(transform.Translation + rotationMatrix * glm::vec3(halfExtent.x, -halfExtent.y, halfExtent.z));
			glm::vec3 vertex7 = glm::vec3(transform.Translation + rotationMatrix * glm::vec3(halfExtent.x, halfExtent.y, halfExtent.z));
			glm::vec3 vertex8 = glm::vec3(transform.Translation + rotationMatrix * glm::vec3(-halfExtent.x, halfExtent.y, halfExtent.z));

			Renderer2D::AddLine(vertex1, vertex2, greenColor); // Edge 1
			Renderer2D::AddLine(vertex2, vertex3, greenColor); // Edge 2
			Renderer2D::AddLine(vertex3, vertex4, greenColor); // Edge 3
			Renderer2D::AddLine(vertex4, vertex1, greenColor); // Edge 4
			Renderer2D::AddLine(vertex5, vertex6, greenColor); // Edge 5
			Renderer2D::AddLine(vertex6, vertex7, greenColor); // Edge 6
			Renderer2D::AddLine(vertex7, vertex8, greenColor); // Edge 7
			Renderer2D::AddLine(vertex8, vertex5, greenColor); // Edge 8
			Renderer2D::AddLine(vertex1, vertex5, greenColor); // Edge 9
			Renderer2D::AddLine(vertex2, vertex6, greenColor); // Edge 10
			Renderer2D::AddLine(vertex3, vertex7, greenColor); // Edge 11
			Renderer2D::AddLine(vertex4, vertex8, greenColor); // Edge 12

			// Diagonals
			Renderer2D::AddLine(vertex1, vertex6, greenColor);
			Renderer2D::AddLine(vertex2, vertex7, greenColor);
			Renderer2D::AddLine(vertex3, vertex8, greenColor);
			Renderer2D::AddLine(vertex4, vertex5, greenColor);
			Renderer2D::AddLine(vertex3, vertex1, greenColor);
			Renderer2D::AddLine(vertex7, vertex5, greenColor);
		}
		else if (auto* sphereCollider = scene->m_Registry->try_get<SphereColliderComponent>(entity); sphereCollider)
		{
			// Define the radius of the sphere
			float radius = sphereCollider->Radius * glm::max(glm::max(transform.Scale.x, transform.Scale.y), transform.Scale.z) + bias;

			// Define the number of lines
			int numLines = 32;

			// Calculate the angle between each line segment
			float segmentAngle = 2 * M_PI / numLines;

			glm::vec3 vertices[2] = {};
			vertices[0] = { radius, 0.0f, 0.0f };

			uint8_t index = 1;
			// Calculate the vertices for the circle
			for (int i = 0; i < numLines; i++)
			{
				float theta = i * segmentAngle;
				vertices[index].x = radius * cos(theta);
				vertices[index].y = radius * sin(theta);
				vertices[index].z = 0.0f;

				const auto& pos = vertices[(index + 1) % 2];
				const auto& pos2 = vertices[index];
				Renderer2D::AddLine(pos + transform.Translation, pos2 + transform.Translation, greenColor);
				Renderer2D::AddLine(glm::vec3(pos.x, pos.z, pos.y) + transform.Translation, glm::vec3(pos2.x, pos2.z, pos2.y) + transform.Translation, greenColor);
				Renderer2D::AddLine(glm::vec3(pos.z, pos.y, pos.x) + transform.Translation, glm::vec3(pos2.z, pos2.y, pos2.x) + transform.Translation, greenColor);
				index = (index + 1) % 2;
			}

			const auto& pos = vertices[(index + 1) % 2];
			Renderer2D::AddLine(pos + transform.Translation, transform.Translation + glm::vec3(radius, 0.0f, 0.0f), greenColor);
			Renderer2D::AddLine(glm::vec3(pos.x, pos.z, pos.y) + transform.Translation, transform.Translation + glm::vec3(radius, 0.0f, 0.0f), greenColor);
			Renderer2D::AddLine(glm::vec3(pos.z, pos.y, pos.x) + transform.Translation, transform.Translation + glm::vec3(0.0f, 0.0f, radius), greenColor);
		}
		else if (auto* capsuleCollider = scene->m_Registry->try_get<CapsuleColliderComponent>(entity); capsuleCollider)
		{
			// Define the radius and half height of the capsule
			float halfHeight = 0.5f * capsuleCollider->Height * transform.Scale.y;
			float radius = capsuleCollider->Radius * glm::max(transform.Scale.x, transform.Scale.z) + bias;

			Renderer2D::AddLine(transform.Translation + rotationMatrix * glm::vec3(radius, halfHeight, 0), transform.Translation + rotationMatrix * glm::vec3(radius, -halfHeight, 0), greenColor);
			Renderer2D::AddLine(transform.Translation + rotationMatrix * glm::vec3(-radius, halfHeight, 0), transform.Translation + rotationMatrix * glm::vec3(-radius, -halfHeight, 0), greenColor);
			Renderer2D::AddLine(transform.Translation + rotationMatrix * glm::vec3(0, halfHeight, radius), transform.Translation + rotationMatrix * glm::vec3(0, -halfHeight, radius), greenColor);
			Renderer2D::AddLine(transform.Translation + rotationMatrix * glm::vec3(0, halfHeight, -radius), transform.Translation + rotationMatrix * glm::vec3(0, -halfHeight, -radius), greenColor);

			Renderer2D::AddCircle(transform.Translation + rotationMatrix * glm::vec3(0, halfHeight, 0), radius, glm::quat(transform.Rotation), greenColor);
			Renderer2D::AddCircle(transform.Translation + rotationMatrix * glm::vec3(0, -halfHeight, 0), radius, glm::quat(transform.Rotation), greenColor);

			// Hemispheres
			Renderer2D::AddSemiCircle(transform.Translation + rotationMatrix * glm::vec3(0, halfHeight, 0), radius, glm::quat(glm::vec3(glm::pi<float>() / 2.0f, 0, glm::pi<float>())), greenColor);
			Renderer2D::AddSemiCircle(transform.Translation + rotationMatrix * glm::vec3(0, halfHeight, 0), radius, glm::quat(glm::vec3(glm::pi<float>() / 2.0f, glm::pi<float>() / 2.0f, glm::pi<float>())), greenColor);
			Renderer2D::AddSemiCircle(transform.Translation - rotationMatrix * glm::vec3(0, halfHeight, 0), radius, glm::quat(glm::vec3(glm::pi<float>() / 2.0f, 0, 0)), greenColor);
			Renderer2D::AddSemiCircle(transform.Translation - rotationMatrix * glm::vec3(0, halfHeight, 0), radius, glm::quat(glm::vec3(glm::pi<float>() / 2.0f, glm::pi<float>() / 2.0f, 0)), greenColor);
		}
	}

	// TODO: Move this to EditorLayer.cpp ASAP
	void SceneRenderer::SubmitCameraViewGeometry(const Ref<Scene>& scene, fbentt::entity entity, TransformComponent& transform)
	{
		constexpr glm::vec3 color(0.961f, 0.796f, 0.486f); // TODO: Replace with Theme::AccentColor
		auto* cameraComp = scene->m_Registry->try_get<CameraComponent>(entity);
		if (cameraComp)
		{
			const glm::mat3 rotationMatrix = glm::toMat3(glm::quat(transform.Rotation));
			const auto& settings = cameraComp->Camera.GetSettings();
			float aspectRatio = m_ViewportSize.x / m_ViewportSize.y;

			switch (settings.ProjectionType)
			{
				case ProjectionType::Orthographic:
				{
					const float left = -aspectRatio * settings.Zoom;
					const float right = -left;
					const float bottom = -settings.Zoom;
					const float top = settings.Zoom;

					Renderer2D::AddLine(rotationMatrix * glm::vec3(left, bottom, settings.Near) + transform.Translation, rotationMatrix * glm::vec3(left, top, settings.Near) + transform.Translation, color);
					Renderer2D::AddLine(rotationMatrix * glm::vec3(left, top, settings.Near) + transform.Translation, rotationMatrix * glm::vec3(right, top, settings.Near) + transform.Translation, color);
					Renderer2D::AddLine(rotationMatrix * glm::vec3(right, top, settings.Near) + transform.Translation, rotationMatrix * glm::vec3(right, bottom, settings.Near) + transform.Translation, color);
					Renderer2D::AddLine(rotationMatrix * glm::vec3(right, bottom, settings.Near) + transform.Translation, rotationMatrix * glm::vec3(left, bottom, settings.Near) + transform.Translation, color);

					Renderer2D::AddLine(rotationMatrix * glm::vec3(left, bottom, settings.Far) + transform.Translation, rotationMatrix * glm::vec3(left, top, settings.Far) + transform.Translation, color);
					Renderer2D::AddLine(rotationMatrix * glm::vec3(left, top, settings.Far) + transform.Translation, rotationMatrix * glm::vec3(right, top, settings.Far) + transform.Translation, color);
					Renderer2D::AddLine(rotationMatrix * glm::vec3(right, top, settings.Far) + transform.Translation, rotationMatrix * glm::vec3(right, bottom, settings.Far) + transform.Translation, color);
					Renderer2D::AddLine(rotationMatrix * glm::vec3(right, bottom, settings.Far) + transform.Translation, rotationMatrix * glm::vec3(left, bottom, settings.Far) + transform.Translation, color);

					Renderer2D::AddLine(rotationMatrix * glm::vec3(left, bottom, settings.Near) + transform.Translation, rotationMatrix * glm::vec3(left, bottom, settings.Far) + transform.Translation, color);
					Renderer2D::AddLine(rotationMatrix * glm::vec3(left, top, settings.Near) + transform.Translation, rotationMatrix * glm::vec3(left, top, settings.Far) + transform.Translation, color);
					Renderer2D::AddLine(rotationMatrix * glm::vec3(right, top, settings.Near) + transform.Translation, rotationMatrix * glm::vec3(right, top, settings.Far) + transform.Translation, color);
					Renderer2D::AddLine(rotationMatrix * glm::vec3(right, bottom, settings.Near) + transform.Translation, rotationMatrix * glm::vec3(right, bottom, settings.Far) + transform.Translation, color);
					break;
				}
				case ProjectionType::Perspective:
				{
					float tanTheta = glm::tan(glm::radians(settings.FOV / 2.0f));
					float nearViewHalfHeight = tanTheta * settings.Near;
					float nearViewHalfWidth = nearViewHalfHeight * aspectRatio;
					float farViewHalfHeight = tanTheta * settings.Far;
					float farViewHalfWidth = farViewHalfHeight * aspectRatio;

					// Submit Frustum Geometry
					glm::vec3 frustumCorners[8] = {
						glm::vec3(-nearViewHalfWidth, nearViewHalfHeight, settings.Near),
						glm::vec3(nearViewHalfWidth, nearViewHalfHeight, settings.Near),
						glm::vec3(nearViewHalfWidth, -nearViewHalfHeight, settings.Near),
						glm::vec3(-nearViewHalfWidth, -nearViewHalfHeight, settings.Near),
						glm::vec3(-farViewHalfWidth, farViewHalfHeight, settings.Far),
						glm::vec3(farViewHalfWidth, farViewHalfHeight, settings.Far),
						glm::vec3(farViewHalfWidth, -farViewHalfHeight, settings.Far),
						glm::vec3(-farViewHalfWidth, -farViewHalfHeight, settings.Far)
					};

					for (uint8_t i = 0; i < 8; i++)
						frustumCorners[i] = rotationMatrix * frustumCorners[i] + transform.Translation;

					Renderer2D::AddLine(frustumCorners[0], frustumCorners[1], color);
					Renderer2D::AddLine(frustumCorners[1], frustumCorners[2], color);
					Renderer2D::AddLine(frustumCorners[2], frustumCorners[3], color);
					Renderer2D::AddLine(frustumCorners[3], frustumCorners[0], color);

					Renderer2D::AddLine(frustumCorners[4], frustumCorners[5], color);
					Renderer2D::AddLine(frustumCorners[5], frustumCorners[6], color);
					Renderer2D::AddLine(frustumCorners[6], frustumCorners[7], color);
					Renderer2D::AddLine(frustumCorners[7], frustumCorners[4], color);

					Renderer2D::AddLine(frustumCorners[0], frustumCorners[4], color);
					Renderer2D::AddLine(frustumCorners[1], frustumCorners[5], color);
					Renderer2D::AddLine(frustumCorners[2], frustumCorners[6], color);
					Renderer2D::AddLine(frustumCorners[3], frustumCorners[7], color);
					break;
				}
			}
		}
	}

	SceneRenderer::~SceneRenderer()
	{
		auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		vkDestroySampler(device, m_ShadowMapSampler, nullptr);
	}

} // namespace Flameberry
