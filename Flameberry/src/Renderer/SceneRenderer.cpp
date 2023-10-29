#include "SceneRenderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Core/Core.h"
#include "VulkanDebug.h"
#include "RenderCommand.h"
#include "VulkanContext.h"
#include "Material.h"
#include "Renderer.h"
#include "Renderer2D.h"

#include "Asset/AssetManager.h"

namespace Flameberry {

    struct CameraUniformBufferObject {
        glm::mat4 ViewMatrix, ProjectionMatrix, ViewProjectionMatrix;
    };

    struct SceneRendererSettingsUniform {
        int EnableShadows = 1, ShowCascades = 0, SoftShadows = 1;
    };

    struct SceneUniformBufferData {
        alignas(64) glm::mat4 CascadeViewProjectionMatrices[SceneRendererSettings::CascadeCount];
        alignas(16) float CascadeDepthSplits[SceneRendererSettings::CascadeCount];
        alignas(16) glm::vec3 cameraPosition;
        alignas(16) DirectionalLight directionalLight;
        alignas(16) PointLight PointLights[10];
        alignas(4)  int LightCount = 0;
        alignas(16) SceneRendererSettingsUniform RendererSettings;
    };

    struct MeshData {
        glm::mat4 ModelMatrix;
        glm::vec3 Albedo{ 1.0f };
        float Roughness = 0.2f;
        float Metallic = 0.0f;
        float AlbedoMapEnabled = 0.0f,
            NormalMapEnabled = 0.0f,
            RoughnessMapEnabled = 0.0f,
            AmbientOcclusionMapEnabled = 0.0f,
            MetallicMapEnabled = 0.0f;
    };

    struct OutlinePushConstantData {
        glm::mat4 ModelMatrix;
        glm::vec2 ScreenSize;
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

        // CommandBufferSpecification commandBufferSpec{};

        // m_CommandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
        // for (auto& commandBuffer : m_CommandBuffers)
        // {
        // }

#pragma region ShadowMapResources
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
            for (uint32_t i = 0; i < imageCount; i++)
                shadowMapRenderPassSpec.TargetFramebuffers[i] = Framebuffer::Create(shadowMapFramebufferSpec);

            m_ShadowMapRenderPass = RenderPass::Create(shadowMapRenderPassSpec);

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

            m_ShadowMapDescriptorSetLayout = DescriptorSetLayout::Create(shadowDescSetLayoutSpec);

            DescriptorSetSpecification shadowMapDescSetSpec;
            shadowMapDescSetSpec.Layout = m_ShadowMapDescriptorSetLayout;

            m_ShadowMapDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
            for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
            {
                m_ShadowMapDescriptorSets[i] = DescriptorSet::Create(shadowMapDescSetSpec);
                
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = m_ShadowMapUniformBuffers[i]->GetBuffer();
                bufferInfo.range = bufferSize;
                bufferInfo.offset = 0;

                m_ShadowMapDescriptorSets[i]->WriteBuffer(0, bufferInfo);
                
                m_ShadowMapDescriptorSets[i]->Update();
            }

            PipelineSpecification pipelineSpec{};
            pipelineSpec.PipelineLayout.PushConstants = {
                { VK_SHADER_STAGE_VERTEX_BIT, sizeof(ModelMatrixPushConstantData) },
            };
            pipelineSpec.PipelineLayout.DescriptorSetLayouts = { m_ShadowMapDescriptorSetLayout };

            pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/shadow_map.vert.spv";
            pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/shadow_map.frag.spv";
            pipelineSpec.RenderPass = m_ShadowMapRenderPass;

            pipelineSpec.VertexLayout = { VertexInputAttribute::VEC3F };
            pipelineSpec.VertexInputBindingDescription = MeshVertex::GetBindingDescription();

            pipelineSpec.Viewport.width = SceneRendererSettings::CascadeSize;
            pipelineSpec.Viewport.height = SceneRendererSettings::CascadeSize;
            pipelineSpec.Scissor = { { 0, 0 }, { SceneRendererSettings::CascadeSize, SceneRendererSettings::CascadeSize } };

            pipelineSpec.CullMode = VK_CULL_MODE_NONE; // TODO: This is ineffecient
            pipelineSpec.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            pipelineSpec.DepthClampEnable = true;

            m_ShadowMapPipeline = Pipeline::Create(pipelineSpec);

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
#pragma endregion ShadowMapResources

#pragma region GeometryPassResources
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

            m_CameraBufferDescSetLayout = DescriptorSetLayout::Create(cameraBufferDescLayoutSpec);

            DescriptorSetSpecification cameraBufferDescSetSpec;
            cameraBufferDescSetSpec.Layout = m_CameraBufferDescSetLayout;

            m_CameraBufferDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
            for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
            {
                m_CameraBufferDescriptorSets[i] = DescriptorSet::Create(cameraBufferDescSetSpec);

                VkDescriptorBufferInfo vk_descriptor_buffer_info{};
                vk_descriptor_buffer_info.buffer = m_CameraUniformBuffers[i]->GetBuffer();
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

            for (uint32_t i = 0; i < imageCount; i++)
                sceneRenderPassSpec.TargetFramebuffers[i] = Framebuffer::Create(sceneFramebufferSpec);

            m_GeometryPass = RenderPass::Create(sceneRenderPassSpec);

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
                sceneDescSetLayoutSpec.Bindings.resize(2);
                sceneDescSetLayoutSpec.Bindings[0].binding = 0;
                sceneDescSetLayoutSpec.Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                sceneDescSetLayoutSpec.Bindings[0].descriptorCount = 1;
                sceneDescSetLayoutSpec.Bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                sceneDescSetLayoutSpec.Bindings[1].binding = 1;
                sceneDescSetLayoutSpec.Bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                sceneDescSetLayoutSpec.Bindings[1].descriptorCount = 1;
                sceneDescSetLayoutSpec.Bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                m_SceneDescriptorSetLayout = DescriptorSetLayout::Create(sceneDescSetLayoutSpec);

                DescriptorSetSpecification sceneDescSetSpec;
                sceneDescSetSpec.Layout = m_SceneDescriptorSetLayout;

                m_SceneDataDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
                for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
                {
                    m_SceneDataDescriptorSets[i] = DescriptorSet::Create(sceneDescSetSpec);
                    
                    VkDescriptorBufferInfo bufferInfo{};
                    bufferInfo.range = sizeof(SceneUniformBufferData);
                    bufferInfo.offset=0;
                    bufferInfo.buffer = m_SceneUniformBuffers[i]->GetBuffer();

//                    m_SceneDataDescriptorSets[i]->WriteBuffer(0, {
//                            .buffer = m_SceneUniformBuffers[i]->GetBuffer(),
//                            .range = sizeof(SceneUniformBufferData),
//                            .offset = 0
//                        }
//                    );
                    m_SceneDataDescriptorSets[i]->WriteBuffer(0, bufferInfo);
                    
                    VkDescriptorImageInfo imageInfo{};
                    imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                    imageInfo.imageView = m_ShadowMapRenderPass->GetSpecification().TargetFramebuffers[i]->GetDepthAttachment()->GetImageView();
                    imageInfo.sampler = m_ShadowMapSampler;
                    
                    m_SceneDataDescriptorSets[i]->WriteImage(1, imageInfo);
                    
                    FL_LOG(m_SceneUniformBuffers[i]->GetBuffer());

                    m_SceneDataDescriptorSets[i]->Update();
                }

                PipelineSpecification pipelineSpec{};
                pipelineSpec.PipelineLayout.PushConstants = {
                    { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(MeshData) }
                };

                // Temp
                Material::Init();

                pipelineSpec.PipelineLayout.DescriptorSetLayouts = {
                    m_CameraBufferDescSetLayout,
                    m_SceneDescriptorSetLayout,
                    Material::GetLayout()
                };

                pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/triangle.vert.spv";
                pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/triangle.frag.spv";
                pipelineSpec.RenderPass = m_GeometryPass;

                pipelineSpec.VertexLayout = {
                    VertexInputAttribute::VEC3F, // a_Position
                    VertexInputAttribute::VEC3F, // a_Normal
                    VertexInputAttribute::VEC2F, // a_TextureCoords
                    VertexInputAttribute::VEC3F, // a_Tangent
                    VertexInputAttribute::VEC3F  // a_BiTangent
                };
                pipelineSpec.VertexInputBindingDescription = MeshVertex::GetBindingDescription();
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

                m_MeshPipeline = Pipeline::Create(pipelineSpec);
            }

            // Outline Resources
            {
                PipelineSpecification pipelineSpec{};
                pipelineSpec.PipelineLayout.PushConstants = {
                    { VK_SHADER_STAGE_VERTEX_BIT, sizeof(OutlinePushConstantData) }
                };

                pipelineSpec.PipelineLayout.DescriptorSetLayouts = { m_CameraBufferDescSetLayout };

                pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/outline.vert.spv";
                pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/outline.frag.spv";
                pipelineSpec.RenderPass = m_GeometryPass;

                pipelineSpec.VertexLayout = {
                    VertexInputAttribute::VEC3F, // a_Position
                    VertexInputAttribute::VEC3F  // a_Normal
                };
                pipelineSpec.VertexInputBindingDescription = MeshVertex::GetBindingDescription();
                pipelineSpec.Samples = RenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());

                pipelineSpec.DepthTestEnable = false;

                pipelineSpec.StencilTestEnable = true;
                pipelineSpec.StencilOpState.failOp = VK_STENCIL_OP_KEEP;
                pipelineSpec.StencilOpState.depthFailOp = VK_STENCIL_OP_KEEP;
                pipelineSpec.StencilOpState.passOp = VK_STENCIL_OP_REPLACE;
                pipelineSpec.StencilOpState.compareOp = VK_COMPARE_OP_NOT_EQUAL;
                pipelineSpec.StencilOpState.compareMask = 0xFF;
                pipelineSpec.StencilOpState.reference = 1;
                pipelineSpec.StencilOpState.writeMask = 1;

                m_OutlinePipeline = Pipeline::Create(pipelineSpec);
            }

            // Skybox Pipeline
            {
                Flameberry::PipelineSpecification pipelineSpec{};
                pipelineSpec.PipelineLayout.PushConstants = {
                    { VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4) }
                };

                pipelineSpec.PipelineLayout.DescriptorSetLayouts = { Texture2D::GetDescriptorLayout() };

                pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/skybox.vert.spv";
                pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/skybox.frag.spv";
                pipelineSpec.RenderPass = m_GeometryPass;

                pipelineSpec.VertexLayout = {};

                pipelineSpec.VertexInputBindingDescription.binding = 0;
                pipelineSpec.VertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                pipelineSpec.VertexInputBindingDescription.stride = 0;

                VkSampleCountFlagBits sampleCount = RenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());
                pipelineSpec.Samples = sampleCount;

                pipelineSpec.CullMode = VK_CULL_MODE_NONE;

                pipelineSpec.DepthTestEnable = true;
                pipelineSpec.DepthWriteEnable = false;
                pipelineSpec.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

                m_SkyboxPipeline = Pipeline::Create(pipelineSpec);
            }
            m_VkTextureSampler = Texture2D::GetDefaultSampler();
        }
#pragma endregion GeometryPassResources

#pragma region CompositionResources
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
                renderPassSpec.TargetFramebuffers[i] = Framebuffer::Create(framebufferSpec);

            m_CompositePass = RenderPass::Create(renderPassSpec);

            DescriptorSetLayoutSpecification layoutSpec;
            layoutSpec.Bindings.resize(1);

            layoutSpec.Bindings[0].binding = 0;
            layoutSpec.Bindings[0].descriptorCount = 1;
            layoutSpec.Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            layoutSpec.Bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            m_CompositePassDescriptorSetLayout = DescriptorSetLayout::Create(layoutSpec);

            DescriptorSetSpecification setSpec;
            setSpec.Layout = m_CompositePassDescriptorSetLayout;

            m_CompositePassDescriptorSets.resize(imageCount);
            for (uint8_t i = 0; i < m_CompositePassDescriptorSets.size(); i++)
            {
                m_CompositePassDescriptorSets[i] = DescriptorSet::Create(setSpec);
                
                VkDescriptorImageInfo imageInfo{
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = m_GeometryPass->GetSpecification().TargetFramebuffers[i]->GetColorResolveAttachment(0)->GetImageView(),
                    .sampler = m_VkTextureSampler
                };
                
                m_CompositePassDescriptorSets[i]->WriteImage(0, imageInfo);

                m_CompositePassDescriptorSets[i]->Update();
            }

            PipelineSpecification pipelineSpec{};
            pipelineSpec.PipelineLayout.PushConstants = {};
            pipelineSpec.PipelineLayout.DescriptorSetLayouts = { m_CompositePassDescriptorSetLayout };

            pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/composite.vert.spv";
            pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/composite.frag.spv";
            pipelineSpec.RenderPass = m_CompositePass;

            pipelineSpec.VertexLayout = {};
            pipelineSpec.VertexInputBindingDescription.binding = 0;
            pipelineSpec.VertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            pipelineSpec.VertexInputBindingDescription.stride = 0;

            pipelineSpec.BlendingEnable = true;
            pipelineSpec.CullMode = VK_CULL_MODE_FRONT_BIT;

            m_CompositePipeline = Pipeline::Create(pipelineSpec);
        }
#pragma endregion CompositionResources

        Renderer2D::Init(m_CameraBufferDescSetLayout, m_GeometryPass);

        // Textures
        m_LightIcon = Texture2D::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/icons/bulb_icon_v4.png");
        m_CameraIcon = Texture2D::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/icons/camera_icon.png");
    }

    void SceneRenderer::RenderScene(const glm::vec2& viewportSize, const std::shared_ptr<Scene>& scene, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec3& cameraPosition, float cameraNear, float cameraFar, fbentt::entity selectedEntity, bool renderGrid, bool renderDebugIcons, bool renderOutline, bool renderPhysicsCollider)
    {
        uint32_t currentFrame = Renderer::GetCurrentFrameIndex();
        std::vector<fbentt::entity> lightEntityHandles;

        m_ViewportSize = viewportSize;

        // Resize Framebuffers
        Renderer::Submit([&](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                const auto& framebufferSpec = m_GeometryPass->GetSpecification().TargetFramebuffers[imageIndex]->GetSpecification();
                if (!(m_ViewportSize.x == 0 || m_ViewportSize.y == 0) && (framebufferSpec.Width != m_ViewportSize.x || framebufferSpec.Height != m_ViewportSize.y))
                {
                    m_GeometryPass->GetSpecification().TargetFramebuffers[imageIndex]->OnResize(m_ViewportSize.x, m_ViewportSize.y, m_GeometryPass->GetRenderPass());

                    VkDescriptorImageInfo imageInfo{
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        .imageView = m_GeometryPass->GetSpecification().TargetFramebuffers[imageIndex]->GetColorResolveAttachment(0)->GetImageView(),
                        .sampler = m_VkTextureSampler
                    };
                    
                    m_CompositePassDescriptorSets[imageIndex]->WriteImage(0, imageInfo);

                    m_CompositePassDescriptorSets[imageIndex]->Update();
                    m_CompositePass->GetSpecification().TargetFramebuffers[imageIndex]->OnResize(m_ViewportSize.x, m_ViewportSize.y, m_CompositePass->GetRenderPass());
                }

//                VkClearColorValue color = { scene->GetClearColor().x, scene->GetClearColor().y, scene->GetClearColor().z, 1.0f };
//                m_GeometryPass->GetSpecification().TargetFramebuffers[imageIndex]->SetClearColorValue(color);
            }
        );

        // Update uniform buffers
        CameraUniformBufferObject cameraBufferData;
        cameraBufferData.ViewMatrix = viewMatrix;
        cameraBufferData.ProjectionMatrix = projectionMatrix;
        cameraBufferData.ViewProjectionMatrix = projectionMatrix * viewMatrix;

        m_CameraUniformBuffers[currentFrame]->WriteToBuffer(&cameraBufferData, sizeof(CameraUniformBufferObject));
        
        SceneUniformBufferData sceneUniformBufferData;
        for (const auto& entity : scene->m_Registry->view<TransformComponent, DirectionalLightComponent>())
        {
            auto [transform, dirLight] = scene->m_Registry->get<TransformComponent, DirectionalLightComponent>(entity);
            sceneUniformBufferData.directionalLight.Color = dirLight.Color;
            sceneUniformBufferData.directionalLight.Intensity = dirLight.Intensity;
            sceneUniformBufferData.directionalLight.Direction = -transform.Translation;
        }

        if (m_RendererSettings.EnableShadows)
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
        sceneUniformBufferData.RendererSettings.EnableShadows = (int)m_RendererSettings.EnableShadows;
        sceneUniformBufferData.RendererSettings.ShowCascades = (int)m_RendererSettings.ShowCascades;
        sceneUniformBufferData.RendererSettings.SoftShadows = (int)m_RendererSettings.SoftShadows;

        for (const auto& entity : scene->m_Registry->view<TransformComponent, PointLightComponent>())
        {
            const auto& [transform, light] = scene->m_Registry->get<TransformComponent, PointLightComponent>(entity);
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Position = transform.Translation;
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Color = light.Color;
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Intensity = light.Intensity;
            sceneUniformBufferData.LightCount++;

            lightEntityHandles.emplace_back(entity);
        }
        
        m_SceneUniformBuffers[currentFrame]->WriteToBuffer(&sceneUniformBufferData, sizeof(SceneUniformBufferData));

        // Render Passes
#pragma region ShadowPass
        if (m_RendererSettings.EnableShadows)
        {
            m_ShadowMapRenderPass->Begin();
            m_ShadowMapPipeline->Bind();
            Renderer::Submit([shadowMapDescSet = m_ShadowMapDescriptorSets[currentFrame]->GetDescriptorSet(), shadowMapPipelineLayout = m_ShadowMapPipeline->GetLayout()](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                {
                    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipelineLayout, 0, 1, &shadowMapDescSet, 0, nullptr);
                }
            );

            for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
            {
                const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);
                auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle);
                if (staticMesh)
                {
                    ModelMatrixPushConstantData pushContantData;
                    pushContantData.ModelMatrix = transform.GetTransform();
                    Renderer::Submit([shadowMapPipelineLayout = m_ShadowMapPipeline->GetLayout(), pushContantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                        {
                            vkCmdPushConstants(cmdBuffer, shadowMapPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrixPushConstantData), &pushContantData);
                        }
                    );
                    staticMesh->Bind();
                    staticMesh->OnDraw();
                }
            }
            m_ShadowMapRenderPass->End();
        }
#pragma endregion ShadowPass

#pragma region GeometryPass
        m_GeometryPass->Begin();

        RenderCommand::SetViewport(0.0f, 0.0f, m_ViewportSize.x, m_ViewportSize.y);
        RenderCommand::SetScissor({ 0, 0 }, VkExtent2D{ (uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y });

        SkyLightComponent* skyMap = nullptr;
        for (const auto entity : scene->m_Registry->view<TransformComponent, SkyLightComponent>())
            skyMap = scene->m_Registry->try_get<SkyLightComponent>(entity);
        
        // Skybox Rendering
        if (skyMap && skyMap->EnableSkyMap && skyMap->SkyMap)
        {
            m_SkyboxPipeline->Bind();

            glm::mat4 viewProjectionMatrix = projectionMatrix * glm::mat4(glm::mat3(viewMatrix));
            auto pipelineLayout = m_SkyboxPipeline->GetLayout();
            auto textureDescSet = AssetManager::GetAsset<Texture2D>(skyMap->SkyMap)->CreateOrGetDescriptorSet();

            Renderer::Submit([pipelineLayout, viewProjectionMatrix, textureDescSet](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                {
                    VkDescriptorSet descSets[] = { textureDescSet };
                    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, descSets, 0, nullptr);
                    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewProjectionMatrix);
                    vkCmdDraw(cmdBuffer, 36, 1, 0, 0);
                }
            );
        }

        // Mesh Rendering
        m_MeshPipeline->Bind();

        VkDescriptorSet descriptorSets[] = {
            m_CameraBufferDescriptorSets[currentFrame]->GetDescriptorSet(),
            m_SceneDataDescriptorSets[currentFrame]->GetDescriptorSet()
        };
        Renderer::Submit([pipelineLayout = m_MeshPipeline->GetLayout(), descriptorSets](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, sizeof(descriptorSets) / sizeof(VkDescriptorSet), descriptorSets, 0, nullptr);
            }
        );

        // Render All Scene Meshes
        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            if (entity == selectedEntity)
                continue;

            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);
            SubmitMesh(mesh.MeshHandle, mesh.OverridenMaterialTable, transform.GetTransform());
        }

        // Render Outlined Object
        if (renderOutline && selectedEntity != fbentt::null)
        {
            auto& transform = scene->m_Registry->get<TransformComponent>(selectedEntity);
            if (scene->m_Registry->has<MeshComponent>(selectedEntity))
            {
                auto& mesh = scene->m_Registry->get<MeshComponent>(selectedEntity);
                auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle);
                if (staticMesh)
                {
                    Renderer::Submit([framebufferSize = m_ViewportSize](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                        {
                            VkClearAttachment attachment{};
                            attachment.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
                            attachment.clearValue = { 1.0f, 0 };
                            attachment.colorAttachment = 1;

                            VkClearRect rect{};
                            rect.baseArrayLayer = 0;
                            rect.layerCount = 1;
                            rect.rect = { {0, 0}, { (uint32_t)framebufferSize.x, (uint32_t)framebufferSize.y } };

                            vkCmdClearAttachments(cmdBuffer, 1, &attachment, 1, &rect);
                        }
                    );
                    // Draw outlined object normally first
                    SubmitMesh(mesh.MeshHandle, mesh.OverridenMaterialTable, transform.GetTransform());

                    // Render Mesh Outline
                    m_OutlinePipeline->Bind();

                    OutlinePushConstantData pushConstantData;
                    pushConstantData.ModelMatrix = glm::scale(transform.GetTransform(), glm::vec3(1.05f));
                    pushConstantData.ScreenSize = m_ViewportSize;

                    Renderer::Submit([outlinePipelineLayout = m_OutlinePipeline->GetLayout(), globalDescriptorSet = m_CameraBufferDescriptorSets[currentFrame]->GetDescriptorSet(), pushConstantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                        {
                            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, outlinePipelineLayout, 0, 1, &globalDescriptorSet, 0, nullptr);
                            vkCmdPushConstants(cmdBuffer, outlinePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(OutlinePushConstantData), &pushConstantData);
                        }
                    );
                    staticMesh->OnDraw();
                }
            }
        }

        Renderer2D::BeginScene(m_CameraBufferDescriptorSets[currentFrame]->GetDescriptorSet());

        if (renderDebugIcons)
        {
            // Render Point Lights
            Renderer2D::SetActiveTexture(m_LightIcon);
            for (uint32_t i = 0; i < sceneUniformBufferData.LightCount; i++)
                Renderer2D::AddBillboard(sceneUniformBufferData.PointLights[i].Position, 0.7f, sceneUniformBufferData.PointLights[i].Color, viewMatrix, fbentt::to_index(lightEntityHandles[i]));
            Renderer2D::FlushQuads();

            Renderer2D::SetActiveTexture(m_CameraIcon);
            for (auto entity : scene->m_Registry->view<TransformComponent, CameraComponent>())
            {
                auto& transform = scene->m_Registry->get<TransformComponent>(entity);
                Renderer2D::AddBillboard(transform.Translation, 0.7f, glm::vec3(1), viewMatrix, fbentt::to_index(entity));
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

        Renderer2D::EndScene();

        m_GeometryPass->End();
#pragma endregion GeometryPass

#pragma region CompositePass
        // m_CompositePass->Begin();
        // m_CompositePipeline->Bind();

        // std::vector<VkDescriptorSet> descSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
        // for (uint8_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        //     descSets[i] = m_CompositePassDescriptorSets[i]->GetDescriptorSet();

        // Renderer::Submit([pipelineLayout = m_CompositePipeline->GetLayout(), descSets](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
        //     {
        //         vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSets[imageIndex], 0, nullptr);
        //         vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
        //     }
        // );
        // m_CompositePass->End();
#pragma endregion CompositePass
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
        for (uint32_t i = 0; i < SceneRendererSettings::CascadeCount; i++) {
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

        for (uint32_t i = 0; i < SceneRendererSettings::CascadeCount; i++) {
            const float splitDist = cascadeSplits[i];

            glm::vec3 frustumCorners[8] = {
                glm::vec3(-1.0f,  1.0f, 0.0f),
                glm::vec3(1.0f,   1.0f, 0.0f),
                glm::vec3(1.0f,  -1.0f, 0.0f),
                glm::vec3(-1.0f, -1.0f, 0.0f),
                glm::vec3(-1.0f,  1.0f, 1.0f),
                glm::vec3(1.0f,   1.0f, 1.0f),
                glm::vec3(1.0f,  -1.0f, 1.0f),
                glm::vec3(-1.0f, -1.0f, 1.0f)
            };

            for (uint32_t i = 0; i < 8; i++) {
                glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
                frustumCorners[i] = invCorner / invCorner.w;
            }

            for (uint32_t i = 0; i < 4; i++) {
                glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
                frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
                frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
            }

            // Get frustum center
            glm::vec3 frustumCenter = glm::vec3(0.0f);
            for (uint32_t i = 0; i < 8; i++) {
                frustumCenter += frustumCorners[i];
            }
            frustumCenter /= 8.0f;

            float radius = 0.0f;
            for (uint32_t i = 0; i < 8; i++) {
                float distance = glm::length(frustumCorners[i] - frustumCenter);
                radius = glm::max(radius, distance);
            }
            //            radius = std::ceil(radius * 16.0f) / 16.0f;

            const float width = frustumCorners[5].x - frustumCorners[4].x;
            const float height = frustumCorners[7].y - frustumCorners[5].y;

            //            const float fWorldUnitsPerTexel = 2.0f * radius * 0.70710678118f / (float)SceneRendererSettings::CascadeSize;
            //            const float fInvWorldUnitsPerTexel = 1.0f / fWorldUnitsPerTexel;

            const glm::vec3 vWorldUnitsPerTexel(width / (float)SceneRendererSettings::CascadeSize, height / (float)SceneRendererSettings::CascadeSize, 1.0f);
            const glm::vec3 vInvWorldUnitsPerTexel = 1.0f / vWorldUnitsPerTexel;

            glm::vec3 maxExtents = (glm::floor(glm::vec3(radius)) * vInvWorldUnitsPerTexel) * vWorldUnitsPerTexel;
            frustumCenter = (glm::floor(frustumCenter) * vInvWorldUnitsPerTexel) * vWorldUnitsPerTexel;
            glm::vec3 minExtents = -maxExtents;

            const glm::vec3 lightDir = glm::normalize(lightDirection);
            const glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter + lightDir * minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

            // Store split distance and matrix in cascade
            m_Cascades[i].DepthSplit = (cameraNear + splitDist * clipRange) * -1.0f;
            m_Cascades[i].ViewProjectionMatrix = lightOrthoMatrix * lightViewMatrix;

            lastSplitDist = cascadeSplits[i];
        }
    }

    void SceneRenderer::RenderSceneForMousePicking(const std::shared_ptr<Scene>& scene, const std::shared_ptr<RenderPass>& renderPass, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Pipeline>& pipeline2D, const glm::vec2& mousePos)
    {
        renderPass->Begin(0, { (int)mousePos.x, (int)mousePos.y }, { 1, 1 });
        pipeline->Bind();

        Renderer::Submit([descSet = m_CameraBufferDescriptorSets[Renderer::GetCurrentFrameIndex()]->GetDescriptorSet(), mousePickingPipelineLayout = pipeline->GetLayout()](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mousePickingPipelineLayout, 0, 1, &descSet, 0, nullptr);
            }
        );

        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);
            auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle);
            if (staticMesh)
            {
                MousePickingPushConstantData pushContantData;
                pushContantData.ModelMatrix = transform.GetTransform();
                pushContantData.EntityIndex = fbentt::to_index(entity);

                Renderer::Submit([mousePickingPipelineLayout = pipeline->GetLayout(), pushContantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                    {
                        vkCmdPushConstants(cmdBuffer, mousePickingPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MousePickingPushConstantData), &pushContantData);
                    }
                );

                staticMesh->Bind();
                staticMesh->OnDraw();
            }
        }

        pipeline2D->Bind();

        uint32_t indexCount = 6 * Renderer2D::GetRendererData().VertexBufferOffset / (4 * sizeof(QuadVertex));
        Renderer::Submit([
            descSet = m_CameraBufferDescriptorSets[Renderer::GetCurrentFrameIndex()]->GetDescriptorSet(),
                mousePicking2DPipelineLayout = pipeline2D->GetLayout(),
                vertexBuffer = Renderer2D::GetRendererData().QuadVertexBuffer->GetBuffer(),
                indexBuffer = Renderer2D::GetRendererData().QuadIndexBuffer->GetBuffer(),
                indexCount]
                (VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mousePicking2DPipelineLayout, 0, 1, &descSet, 0, nullptr);

                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);
                vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);
            }
        );
        renderPass->End();
    }

    void SceneRenderer::ReloadMeshShaders()
    {
        m_MeshPipeline->ReloadShaders();
    }

    void SceneRenderer::SubmitMesh(AssetHandle handle, const MaterialTable& materialTable, const glm::mat4& transform)
    {
        auto staticMesh = AssetManager::GetAsset<StaticMesh>(handle);
        if (!staticMesh)
            return;

        staticMesh->Bind();

        int submeshIndex = 0;
        for (const auto& submesh : staticMesh->GetSubMeshes()) {
            MeshData pushConstantMeshData;
            pushConstantMeshData.ModelMatrix = transform;

            std::shared_ptr<Material> materialAsset;
            if (auto it = materialTable.find(submeshIndex); it != materialTable.end())
                materialAsset = AssetManager::GetAsset<Material>(it->second);
            else if (AssetManager::IsAssetHandleValid(submesh.MaterialHandle))
                materialAsset = AssetManager::GetAsset<Material>(submesh.MaterialHandle);

            if (materialAsset)
            {
                pushConstantMeshData.Albedo = materialAsset->GetAlbedo();
                pushConstantMeshData.Roughness = materialAsset->GetRoughness();
                pushConstantMeshData.Metallic = materialAsset->GetMetallic();
                pushConstantMeshData.AlbedoMapEnabled = materialAsset->IsAlbedoMapEnabled();
                pushConstantMeshData.NormalMapEnabled = materialAsset->IsNormalMapEnabled();
                pushConstantMeshData.RoughnessMapEnabled = materialAsset->IsRoughnessMapEnabled();
                pushConstantMeshData.AmbientOcclusionMapEnabled = materialAsset->IsAmbientOcclusionMapEnabled();
                pushConstantMeshData.MetallicMapEnabled = materialAsset->IsMetallicMapEnabled();
            }

            Renderer::Submit([pipelineLayout = m_MeshPipeline->GetLayout(), descSet = materialAsset ? materialAsset->GetDescriptorSet() : Material::GetEmptyDesciptorSet(), pushConstantMeshData](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                {
                    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &descSet, 0, nullptr);
                    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshData), &pushConstantMeshData);
                }
            );
            staticMesh->OnDrawSubMesh(submeshIndex);
            submeshIndex++;
        }
    }

    void SceneRenderer::SubmitPhysicsColliderGeometry(const std::shared_ptr<Scene>& scene, fbentt::entity entity, TransformComponent& transform)
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
            for (int i = 0; i < numLines; i++) {
                float theta = i * segmentAngle;
                vertices[index].x = radius * cos(theta);
                vertices[index].y = radius * sin(theta);
                vertices[index].z = 0.0f;

                const auto& pos = vertices[(index + 1) % 2];
                const auto& pos2 = vertices[index];
                Renderer2D::AddLine(pos + transform.Translation, pos2 + transform.Translation, greenColor);
                Renderer2D::AddLine(glm::vec3{ pos.x, pos.z, pos.y } + transform.Translation, glm::vec3{ pos2.x, pos2.z, pos2.y } + transform.Translation, greenColor);
                Renderer2D::AddLine(glm::vec3{ pos.z, pos.y, pos.x } + transform.Translation, glm::vec3{ pos2.z, pos2.y, pos2.x } + transform.Translation, greenColor);
                index = (index + 1) % 2;
            }

            const auto& pos = vertices[(index + 1) % 2];
            Renderer2D::AddLine(pos + transform.Translation, transform.Translation + glm::vec3{ radius, 0.0f, 0.0f }, greenColor);
            Renderer2D::AddLine(glm::vec3{ pos.x, pos.z, pos.y } + transform.Translation, transform.Translation + glm::vec3{ radius, 0.0f, 0.0f }, greenColor);
            Renderer2D::AddLine(glm::vec3{ pos.z, pos.y, pos.x } + transform.Translation, transform.Translation + glm::vec3{ 0.0f, 0.0f, radius }, greenColor);
        }
        else if (auto* capsuleCollider = scene->m_Registry->try_get<CapsuleColliderComponent>(entity); capsuleCollider)
        {
            // TODO: Fix the wrong vertices
            // Define the radius and half height of the capsule
            float halfHeight = 0.5f * capsuleCollider->Height * transform.Scale.y;
            float radius = capsuleCollider->Radius * glm::max(transform.Scale.x, transform.Scale.z) + bias;

            Renderer2D::AddLine(transform.Translation + rotationMatrix * glm::vec3(radius, halfHeight, 0), transform.Translation + rotationMatrix * glm::vec3(radius, -halfHeight, 0), greenColor);
            Renderer2D::AddLine(transform.Translation + rotationMatrix * glm::vec3(-radius, halfHeight, 0), transform.Translation + rotationMatrix * glm::vec3(-radius, -halfHeight, 0), greenColor);
            Renderer2D::AddLine(transform.Translation + rotationMatrix * glm::vec3(0, halfHeight, radius), transform.Translation + rotationMatrix * glm::vec3(0, -halfHeight, radius), greenColor);
            Renderer2D::AddLine(transform.Translation + rotationMatrix * glm::vec3(0, halfHeight, -radius), transform.Translation + rotationMatrix * glm::vec3(0, -halfHeight, -radius), greenColor);

            // Define the number of lines
            constexpr int numLines = 32;

            // Calculate the angle between each line segment
            constexpr float segmentAngle = 2 * M_PI / numLines;

            glm::vec3 vertices[2] = {};
            vertices[0] = { radius, 0.0f, 0.0f };

            uint8_t index = 1;
            // Calculate the vertices for the circle
            for (int i = 0; i < numLines; i++)
            {
                float theta = i * segmentAngle;
                vertices[index].x = radius * cos(theta);
                vertices[index].y = 0.0f;
                vertices[index].z = radius * sin(theta);

                const auto& pos = vertices[(index + 1) % 2];
                const auto& pos2 = vertices[index];
                Renderer2D::AddLine(rotationMatrix * (pos + glm::vec3(0, halfHeight, 0)) + transform.Translation, rotationMatrix * (pos2 + glm::vec3(0, halfHeight, 0)) + transform.Translation, greenColor);
                Renderer2D::AddLine(rotationMatrix * (pos + glm::vec3(0, -halfHeight, 0)) + transform.Translation, rotationMatrix * (pos2 + glm::vec3(0, -halfHeight, 0)) + transform.Translation, greenColor);

                // Hemispheres
                const glm::vec3& vertex1 = { vertices[(index + 1) % 2].x, vertices[(index + 1) % 2].z, vertices[(index + 1) % 2].y };
                const glm::vec3& vertex2 = { vertices[index].x, vertices[index].z, vertices[index].y };
                const glm::vec3& vertex3 = { vertices[(index + 1) % 2].y, vertices[(index + 1) % 2].x, vertices[(index + 1) % 2].z };
                const glm::vec3& vertex4 = { vertices[index].y, vertices[index].x, vertices[index].z };
                if (i < numLines / 2)
                {
                    Renderer2D::AddLine(rotationMatrix * (vertex1 + glm::vec3(0, halfHeight, 0)) + transform.Translation, rotationMatrix * (vertex2 + glm::vec3(0, halfHeight, 0)) + transform.Translation, greenColor);
                    Renderer2D::AddLine(rotationMatrix * (vertex3 + glm::vec3(0, halfHeight, 0)) + transform.Translation, rotationMatrix * (vertex4 + glm::vec3(0, halfHeight, 0)) + transform.Translation, greenColor);
                }
                else
                {
                    Renderer2D::AddLine(rotationMatrix * (vertex1 + glm::vec3(0, -halfHeight, 0)) + transform.Translation, rotationMatrix * (vertex2 + glm::vec3(0, -halfHeight, 0)) + transform.Translation, greenColor);
                    Renderer2D::AddLine(rotationMatrix * (vertex3 + glm::vec3(0, -halfHeight, 0)) + transform.Translation, rotationMatrix * (vertex4 + glm::vec3(0, -halfHeight, 0)) + transform.Translation, greenColor);
                }
                index = (index + 1) % 2;
            }

            const auto& pos = vertices[(index + 1) % 2];
            Renderer2D::AddLine(rotationMatrix * (pos + glm::vec3(0, halfHeight, 0)) + transform.Translation, transform.Translation + rotationMatrix * glm::vec3{ radius, halfHeight, 0.0f }, greenColor);
            Renderer2D::AddLine(rotationMatrix * (pos + glm::vec3(0, -halfHeight, 0)) + transform.Translation, transform.Translation + rotationMatrix * glm::vec3{ radius, -halfHeight, 0.0f }, greenColor);

            const glm::vec3& vertex = { vertices[(index + 1) % 2].x, vertices[(index + 1) % 2].z, vertices[(index + 1) % 2].y };
            const glm::vec3& vertex2 = { vertices[(index + 1) % 2].y, vertices[(index + 1) % 2].x, vertices[(index + 1) % 2].z };

            Renderer2D::AddLine(rotationMatrix * (vertex + glm::vec3(0, halfHeight, 0)) + transform.Translation, transform.Translation + rotationMatrix * glm::vec3{ 0.0f, halfHeight, radius }, greenColor);
            Renderer2D::AddLine(rotationMatrix * (vertex2 + glm::vec3(0, -halfHeight, 0)) + transform.Translation, transform.Translation + rotationMatrix * glm::vec3{ 0.0f, -halfHeight, radius }, greenColor);
        }
    }

    void SceneRenderer::SubmitCameraViewGeometry(const std::shared_ptr<Scene>& scene, fbentt::entity entity, TransformComponent& transform)
    {
        constexpr glm::vec3 color(1);
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
                    const float left = -settings.AspectRatio * settings.Zoom;
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
                        glm::vec3(nearViewHalfWidth,  nearViewHalfHeight, settings.Near),
                        glm::vec3(nearViewHalfWidth,  -nearViewHalfHeight, settings.Near),
                        glm::vec3(-nearViewHalfWidth, -nearViewHalfHeight, settings.Near),
                        glm::vec3(-farViewHalfWidth,  farViewHalfHeight, settings.Far),
                        glm::vec3(farViewHalfWidth,   farViewHalfHeight, settings.Far),
                        glm::vec3(farViewHalfWidth,  -farViewHalfHeight, settings.Far),
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

        Material::Shutdown();
    }

}
