#include "SceneRenderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Vulkan/VulkanDebug.h"
#include "Vulkan/RenderCommand.h"
#include "Vulkan/VulkanContext.h"
#include "Material.h"
#include "Renderer.h"
#include "Renderer2D.h"

#include "Asset/AssetManager.h"

namespace Flameberry {
    struct CameraUniformBufferObject {
        glm::mat4 ViewMatrix, ProjectionMatrix, ViewProjectionMatrix;
    };

    struct SceneUniformBufferData {
        alignas(64) glm::mat4 CascadeViewProjectionMatrices[SceneRendererSettings::CascadeCount];
        alignas(16) float CascadeDepthSplits[SceneRendererSettings::CascadeCount];
        alignas(16) glm::vec3 cameraPosition;
        alignas(16) DirectionalLight directionalLight;
        alignas(16) PointLight PointLights[10];
        alignas(4)  int LightCount = 0;
        alignas(4)  int ShowCascades = 0;
    };

    struct MeshData {
        glm::mat4 ModelMatrix;
        glm::vec3 Albedo{ 1.0f };
        float Roughness = 0.2f;
        float Metallic = 0.0f;
        float TextureMapEnabled = 0.0f,
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

            m_ShadowMapUniformBuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
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

            m_ShadowMapDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
            for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
            {
                m_ShadowMapDescriptorSets[i] = DescriptorSet::Create(shadowMapDescSetSpec);

                m_ShadowMapDescriptorSets[i]->WriteBuffer(0, {
                        .buffer = m_ShadowMapUniformBuffers[i]->GetBuffer(),
                        .range = bufferSize,
                        .offset = 0
                    }
                );

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

            // pipelineSpec.CullMode = VK_CULL_MODE_FRONT_BIT;
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

            m_CameraUniformBuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
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

            m_CameraBufferDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
            for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
            {
                m_CameraBufferDescriptorSets[i] = DescriptorSet::Create(cameraBufferDescSetSpec);

                VkDescriptorBufferInfo vk_descriptor_buffer_info{};
                vk_descriptor_buffer_info.buffer = m_CameraUniformBuffers[i]->GetBuffer();
                vk_descriptor_buffer_info.offset = 0;
                vk_descriptor_buffer_info.range = sizeof(CameraUniformBufferObject);

                m_CameraBufferDescriptorSets[i]->WriteBuffer(0, vk_descriptor_buffer_info);
                m_CameraBufferDescriptorSets[i]->Update();
            }

            FramebufferSpecification sceneFramebufferSpec;
            sceneFramebufferSpec.Width = m_ViewportSize.x;
            sceneFramebufferSpec.Height = m_ViewportSize.y;
            sceneFramebufferSpec.Attachments = { swapchainImageFormat, VulkanSwapChain::GetDepthFormat() };
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

                m_SceneUniformBuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
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

                m_SceneDataDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
                for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
                {
                    m_SceneDataDescriptorSets[i] = DescriptorSet::Create(sceneDescSetSpec);

                    m_SceneDataDescriptorSets[i]->WriteBuffer(0, {
                            .buffer = m_SceneUniformBuffers[i]->GetBuffer(),
                            .range = sizeof(SceneUniformBufferData),
                            .offset = 0
                        }
                    );

                    m_SceneDataDescriptorSets[i]->WriteImage(1, {
                            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                            .imageView = m_ShadowMapRenderPass->GetSpecification().TargetFramebuffers[i]->GetDepthAttachment()->GetImageView(),
                            .sampler = m_ShadowMapSampler
                        }
                    );

                    m_SceneDataDescriptorSets[i]->Update();
                }

                PipelineSpecification pipelineSpec{};
                pipelineSpec.PipelineLayout.PushConstants = {
                    { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(MeshData) }
                };

                pipelineSpec.PipelineLayout.DescriptorSetLayouts = {
                    m_CameraBufferDescSetLayout,
                    m_SceneDescriptorSetLayout,
                    Texture2D::GetDescriptorLayout(),
                    Texture2D::GetDescriptorLayout(),
                    Texture2D::GetDescriptorLayout(),
                    Texture2D::GetDescriptorLayout(),
                    Texture2D::GetDescriptorLayout()
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

                m_CompositePassDescriptorSets[i]->WriteImage(0, {
                     .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     .imageView = m_GeometryPass->GetSpecification().TargetFramebuffers[i]->GetColorResolveAttachment(0)->GetImageView(),
                     .sampler = m_VkTextureSampler
                    }
                );

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
    }

    void SceneRenderer::RenderScene(const glm::vec2& viewportSize, const std::shared_ptr<Scene>& scene, const std::shared_ptr<PerspectiveCamera>& camera, fbentt::entity selectedEntity)
    {
        uint32_t currentFrame = Renderer::GetCurrentFrameIndex();
        m_ViewportSize = viewportSize;

        // Resize Framebuffers
        Renderer::Submit([&](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                if (m_GeometryPass->GetSpecification().TargetFramebuffers[imageIndex]->Resize(m_ViewportSize.x, m_ViewportSize.y, m_GeometryPass->GetRenderPass()))
                {
                    m_CompositePassDescriptorSets[imageIndex]->WriteImage(0, {
                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            .imageView = m_GeometryPass->GetSpecification().TargetFramebuffers[imageIndex]->GetColorResolveAttachment(0)->GetImageView(),
                            .sampler = m_VkTextureSampler
                        }
                    );

                    m_CompositePassDescriptorSets[imageIndex]->Update();
                }
                m_CompositePass->GetSpecification().TargetFramebuffers[imageIndex]->Resize(m_ViewportSize.x, m_ViewportSize.y, m_CompositePass->GetRenderPass());

                VkClearColorValue clearColor = { scene->GetClearColor().x, scene->GetClearColor().y, scene->GetClearColor().z, 1.0f };
                m_GeometryPass->GetSpecification().TargetFramebuffers[imageIndex]->SetClearColorValue(clearColor);
            }
        );

        // Update uniform buffers
        CameraUniformBufferObject cameraBufferData;
        cameraBufferData.ViewMatrix = camera->GetViewMatrix();
        cameraBufferData.ProjectionMatrix = camera->GetProjectionMatrix();
        cameraBufferData.ViewProjectionMatrix = camera->GetViewProjectionMatrix();

        m_CameraUniformBuffers[currentFrame]->WriteToBuffer(&cameraBufferData, sizeof(CameraUniformBufferObject));

        CalculateShadowMapCascades(camera, scene->m_SceneData.ActiveEnvironment.DirLight.Direction);
        glm::mat4 cascades[SceneRendererSettings::CascadeCount];
        for (uint8_t i = 0; i < SceneRendererSettings::CascadeCount; i++)
            cascades[i] = m_Cascades[i].ViewProjectionMatrix;

        m_ShadowMapUniformBuffers[currentFrame]->WriteToBuffer(cascades, sizeof(glm::mat4) * SceneRendererSettings::CascadeCount);

        SceneUniformBufferData sceneUniformBufferData;
        sceneUniformBufferData.cameraPosition = camera->GetSpecification().Position;

        for (uint32_t i = 0; i < SceneRendererSettings::CascadeCount; i++)
        {
            sceneUniformBufferData.CascadeViewProjectionMatrices[i] = m_Cascades[i].ViewProjectionMatrix;
            sceneUniformBufferData.CascadeDepthSplits[i] = m_Cascades[i].DepthSplit;
        }
        sceneUniformBufferData.ShowCascades = (int)m_RendererSettings.ShowCascades;

        sceneUniformBufferData.directionalLight = scene->m_SceneData.ActiveEnvironment.DirLight;
        for (const auto& entity : scene->m_Registry->view<TransformComponent, LightComponent>())
        {
            const auto& [transform, light] = scene->m_Registry->get<TransformComponent, LightComponent>(entity);
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Position = transform.translation;
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Color = light.Color;
            sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Intensity = light.Intensity;
            sceneUniformBufferData.LightCount++;
        }

        m_SceneUniformBuffers[currentFrame]->WriteToBuffer(&sceneUniformBufferData, sizeof(SceneUniformBufferData));

        // Render Passes
#pragma region ShadowPass
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
#pragma endregion ShadowPass

#pragma region GeometryPass
        m_GeometryPass->Begin();

        RenderCommand::SetViewport(0.0f, 0.0f, m_ViewportSize.x, m_ViewportSize.y);
        RenderCommand::SetScissor({ 0, 0 }, VkExtent2D{ (uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y });

        // Skybox Rendering
        if (scene->m_SceneData.ActiveEnvironment.EnableEnvironmentMap && scene->m_SceneData.ActiveEnvironment.EnvironmentMap)
        {
            m_SkyboxPipeline->Bind();

            glm::mat4 viewProjectionMatrix = camera->GetProjectionMatrix() * glm::mat4(glm::mat3(camera->GetViewMatrix()));
            auto pipelineLayout = m_SkyboxPipeline->GetLayout();
            auto textureDescSet = scene->m_SceneData.ActiveEnvironment.EnvironmentMap->GetDescriptorSet();

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
        if (selectedEntity != fbentt::null && scene->m_Registry->has<MeshComponent>(selectedEntity))
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(selectedEntity);
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

        // Render Point Lights
        for (uint32_t i = 0; i < sceneUniformBufferData.LightCount; i++)
            Renderer2D::AddBillboard(sceneUniformBufferData.PointLights[i].Position, 0.7f, sceneUniformBufferData.PointLights[i].Color, camera->GetViewMatrix());

        Renderer2D::Render(m_CameraBufferDescriptorSets[currentFrame]->GetDescriptorSet());

        m_GeometryPass->End();
#pragma endregion GeometryPass

#pragma region CompositePass
        m_CompositePass->Begin();
        m_CompositePipeline->Bind();

        std::vector<VkDescriptorSet> descSets(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (uint8_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
            descSets[i] = m_CompositePassDescriptorSets[i]->GetDescriptorSet();

        Renderer::Submit([pipelineLayout = m_CompositePipeline->GetLayout(), descSets](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSets[imageIndex], 0, nullptr);
                vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
            }
        );
        m_CompositePass->End();
#pragma endregion CompositePass
    }

    void SceneRenderer::CalculateShadowMapCascades(const std::shared_ptr<PerspectiveCamera>& camera, const glm::vec3& lightDirection)
    {
        float cascadeSplits[SceneRendererSettings::CascadeCount];

        const float nearClip = camera->GetSpecification().zNear;
        const float farClip = camera->GetSpecification().zFar;
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
        const glm::mat4 invCam = glm::inverse(camera->GetViewProjectionMatrix());

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
                glm::vec3(-1.0f, -1.0f, 1.0f),
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
            radius = std::ceil(radius * 16.0f) / 16.0f;

            const glm::vec3 maxExtents = glm::vec3(radius);
            const glm::vec3 minExtents = -maxExtents;

            const glm::vec3 lightDir = glm::normalize(lightDirection);
            const glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

            // Store split distance and matrix in cascade
            m_Cascades[i].DepthSplit = (camera->GetSpecification().zNear + splitDist * clipRange) * -1.0f;
            m_Cascades[i].ViewProjectionMatrix = lightOrthoMatrix * lightViewMatrix;

            lastSplitDist = cascadeSplits[i];
        }
    }

    // void SceneRenderer::OnDraw(
    //     VkDescriptorSet globalDescriptorSet,
    //     const PerspectiveCamera& activeCamera,
    //     const std::shared_ptr<Scene>& scene,
    //     const glm::vec2& framebufferSize,
    //     const std::array<glm::mat4, SceneRendererSettings::CascadeCount>& cascadeMatrices,
    //     const std::array<float, SceneRendererSettings::CascadeCount>& cascadeSplits,
    //     bool colorCascades,
    //     const fbentt::entity& selectedEntity
    // )
    // {
    //     uint32_t currentFrameIndex = Renderer::GetCurrentFrameIndex();

    //     // Skybox Rendering
    //     if (scene->m_SceneData.ActiveEnvironment.EnableEnvironmentMap && scene->m_SceneData.ActiveEnvironment.EnvironmentMap)
    //     {
    //         m_SkyboxPipeline->Bind();
    //         glm::mat4 viewProjectionMatrix = activeCamera.GetProjectionMatrix() * glm::mat4(glm::mat3(activeCamera.GetViewMatrix()));

    //         auto pipelineLayout = m_SkyboxPipeline->GetLayout();
    //         auto textureDescSet = scene->m_SceneData.ActiveEnvironment.EnvironmentMap->GetDescriptorSet();
    //         Renderer::Submit([pipelineLayout, viewProjectionMatrix, textureDescSet](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
    //             {
    //                 VkDescriptorSet descSets[] = { textureDescSet };
    //                 vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, descSets, 0, nullptr);
    //                 vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewProjectionMatrix);
    //                 vkCmdDraw(cmdBuffer, 36, 1, 0, 0);
    //             }
    //         );
    //     }

    //     m_MeshPipeline->Bind();

    //     // Updating Scene Uniform Buffer
    //     SceneUniformBufferData sceneUniformBufferData;
    //     sceneUniformBufferData.cameraPosition = activeCamera.GetSpecification().Position;
    //     sceneUniformBufferData.directionalLight = scene->m_SceneData.ActiveEnvironment.DirLight;
    //     for (const auto& entity : scene->m_Registry->view<TransformComponent, LightComponent>())
    //     {
    //         const auto& [transform, light] = scene->m_Registry->get<TransformComponent, LightComponent>(entity);
    //         sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Position = transform.translation;
    //         sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Color = light.Color;
    //         sceneUniformBufferData.PointLights[sceneUniformBufferData.LightCount].Intensity = light.Intensity;
    //         sceneUniformBufferData.LightCount++;

    //         Renderer2D::AddBillboard(transform.translation, 0.7f, glm::vec3(1.0f), activeCamera.GetViewMatrix());
    //     }
    //     for (uint32_t i = 0; i < SceneRendererSettings::CascadeCount; i++)
    //     {
    //         sceneUniformBufferData.CascadeViewProjectionMatrices[i] = cascadeMatrices[i];
    //         sceneUniformBufferData.CascadeDepthSplits[i] = cascadeSplits[i];
    //     }
    //     sceneUniformBufferData.ShowCascades = (int)colorCascades;
    //     // sceneUniformBufferData.EnvironmentMapReflections = scene->m_SceneData.ActiveEnvironment.Reflections;

    //     m_SceneUniformBuffers[currentFrameIndex]->WriteToBuffer(&sceneUniformBufferData, sizeof(SceneUniformBufferData), 0);

    //     // Draw Scene Meshes
    //     VkDescriptorSet descriptorSets[] = { globalDescriptorSet, m_SceneDataDescriptorSets[currentFrameIndex]->GetDescriptorSet() };
    //     const auto& pipelineLayout = m_MeshPipeline->GetLayout();

    //     Renderer::Submit([pipelineLayout, descriptorSets](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
    //         {
    //             vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, sizeof(descriptorSets) / sizeof(VkDescriptorSet), descriptorSets, 0, nullptr);
    //         }
    //     );

    //     for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
    //     {
    //         if (entity == selectedEntity)
    //             continue;

    //         const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

    //         SubmitMesh(mesh.MeshHandle, mesh.OverridenMaterialTable, transform.GetTransform());
    //     }

    //     // Draw Outlined Object
    //     if (selectedEntity != fbentt::null && scene->m_Registry->has<MeshComponent>(selectedEntity))
    //     {
    //         const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(selectedEntity);

    //         auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle);
    //         if (staticMesh) {
    //             Renderer::Submit([framebufferSize](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
    //                 {
    //                     VkClearAttachment attachment{};
    //                     attachment.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
    //                     attachment.clearValue = { 1.0f, 0 };
    //                     attachment.colorAttachment = 1;

    //                     VkClearRect rect{};
    //                     rect.baseArrayLayer = 0;
    //                     rect.layerCount = 1;
    //                     rect.rect = { {0, 0}, { (uint32_t)framebufferSize.x, (uint32_t)framebufferSize.y } };

    //                     vkCmdClearAttachments(cmdBuffer, 1, &attachment, 1, &rect);
    //                 }
    //             );

    //             SubmitMesh(mesh.MeshHandle, mesh.OverridenMaterialTable, transform.GetTransform());

    //             m_OutlinePipeline->Bind();

    //             const auto& outlinePipelineLayout = m_OutlinePipeline->GetLayout();

    //             OutlinePushConstantData pushConstantData;
    //             pushConstantData.ModelMatrix = glm::scale(transform.GetTransform(), glm::vec3(1.05f));
    //             // pushConstantData.ModelMatrix = transform.GetTransform();
    //             pushConstantData.ScreenSize = framebufferSize;

    //             Renderer::Submit([outlinePipelineLayout, globalDescriptorSet, pushConstantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
    //                 {
    //                     vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, outlinePipelineLayout, 0, 1, &globalDescriptorSet, 0, nullptr);
    //                     vkCmdPushConstants(cmdBuffer, outlinePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(OutlinePushConstantData), &pushConstantData);
    //                 }
    //             );

    //             staticMesh->OnDraw();
    //         }
    //     }
    // }

    // void SceneRenderer::OnDrawForShadowPass(VkPipelineLayout shadowMapPipelineLayout, const std::shared_ptr<Scene>& scene)
    // {
    //     for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
    //     {
    //         const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

    //         auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle);
    //         if (staticMesh)
    //         {
    //             ModelMatrixPushConstantData pushContantData;
    //             pushContantData.ModelMatrix = transform.GetTransform();
    //             Renderer::Submit([shadowMapPipelineLayout, pushContantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
    //                 {
    //                     vkCmdPushConstants(cmdBuffer, shadowMapPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrixPushConstantData), &pushContantData);
    //                 }
    //             );

    //             staticMesh->Bind();
    //             staticMesh->OnDraw();
    //         }
    //     }
    // }

    void SceneRenderer::RenderSceneForMousePicking(VkPipelineLayout mousePickingPipelineLayout, const std::shared_ptr<Scene>& scene)
    {
        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);
            auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle);
            if (staticMesh)
            {
                MousePickingPushConstantData pushContantData;
                pushContantData.ModelMatrix = transform.GetTransform();
                pushContantData.EntityIndex = fbentt::to_index(entity);

                Renderer::Submit([mousePickingPipelineLayout, pushContantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                    {
                        vkCmdPushConstants(cmdBuffer, mousePickingPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MousePickingPushConstantData), &pushContantData);
                    }
                );

                staticMesh->Bind();
                staticMesh->OnDraw();
            }
        }
    }

    void SceneRenderer::SubmitMesh(AssetHandle handle, const MaterialTable& materialTable, const glm::mat4& transform)
    {
        auto staticMesh = AssetManager::GetAsset<StaticMesh>(handle);
        if (!staticMesh)
            return;

        std::vector<VkDescriptorSet> materialDescriptorSets(5);

        staticMesh->Bind();

        int submeshIndex = 0;
        for (const auto& submesh : staticMesh->GetSubMeshes()) {
            MeshData pushConstantMeshData;
            pushConstantMeshData.ModelMatrix = transform;

            std::shared_ptr<Material> materialAsset;
            std::fill(materialDescriptorSets.begin(), materialDescriptorSets.end(), Texture2D::GetEmptyDescriptorSet());

            if (auto it = materialTable.find(submeshIndex); it != materialTable.end())
                materialAsset = AssetManager::GetAsset<Material>(it->second);
            else if (AssetManager::IsAssetHandleValid(submesh.MaterialHandle))
                materialAsset = AssetManager::GetAsset<Material>(submesh.MaterialHandle);

            if (materialAsset)
            {
                pushConstantMeshData.Albedo = materialAsset->Albedo;
                pushConstantMeshData.Roughness = materialAsset->Roughness;
                pushConstantMeshData.Metallic = materialAsset->Metallic;
                pushConstantMeshData.TextureMapEnabled = materialAsset->TextureMapEnabled;
                pushConstantMeshData.NormalMapEnabled = materialAsset->NormalMapEnabled;
                pushConstantMeshData.RoughnessMapEnabled = materialAsset->RoughnessMapEnabled;
                pushConstantMeshData.AmbientOcclusionMapEnabled = materialAsset->AmbientOcclusionMapEnabled;
                pushConstantMeshData.MetallicMapEnabled = materialAsset->MetallicMapEnabled;

                if (materialAsset->TextureMapEnabled)
                    materialDescriptorSets[0] = materialAsset->TextureMap->GetDescriptorSet();
                if (materialAsset->NormalMapEnabled)
                    materialDescriptorSets[1] = materialAsset->NormalMap->GetDescriptorSet();
                if (materialAsset->RoughnessMapEnabled)
                    materialDescriptorSets[2] = materialAsset->RoughnessMap->GetDescriptorSet();
                if (materialAsset->AmbientOcclusionMapEnabled)
                    materialDescriptorSets[3] = materialAsset->AmbientOcclusionMap->GetDescriptorSet();
                if (materialAsset->MetallicMapEnabled)
                    materialDescriptorSets[4] = materialAsset->MetallicMap->GetDescriptorSet();
            }

            Renderer::Submit([pipelineLayout = m_MeshPipeline->GetLayout(), materialDescriptorSets, pushConstantMeshData](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                {
                    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, materialDescriptorSets.size(), materialDescriptorSets.data(), 0, nullptr);
                    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshData), &pushConstantMeshData);
                }
            );
            staticMesh->OnDrawSubMesh(submeshIndex);
            submeshIndex++;
        }
    }

    SceneRenderer::~SceneRenderer()
    {
    }
}
