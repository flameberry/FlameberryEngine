#include "VulkanRenderer.h"

#include <cstdint>
#include <algorithm>
#include <chrono>
#include <thread>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Core/Core.h"
#include "Core/Timer.h"
#include "VulkanRenderCommand.h"
#include "VulkanTexture.h"
#include "StaticMesh.h"
#include "VulkanDebug.h"

#include "AssetManager/AssetManager.h"

namespace Flameberry {
    VulkanRenderer::VulkanRenderer(VulkanWindow* pWindow)
    {
        VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VkPhysicalDevice physicalDevice = VulkanContext::GetPhysicalDevice();

        m_SwapChain = std::make_unique<VulkanSwapChain>(pWindow->GetWindowSurface());

        VulkanContext::GetCurrentDevice()->AllocateCommandBuffers(m_SwapChain->GetSwapChainImageCount());

        // Create the generic texture descriptor layout
        VulkanTexture::InitStaticResources();

        CreateViewportRenderPass();
        CreateViewportResources();

        CreateMousePickingRenderPass();
        CreateMousePickingResources();

        if (m_EnableShadows)
        {
            m_ShadowMapImages.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
            for (auto& shadowMapImage : m_ShadowMapImages) {
                shadowMapImage = std::make_shared<VulkanImage>(
                    SHADOW_MAP_WIDTH,
                    SHADOW_MAP_HEIGHT,
                    1,
                    VK_FORMAT_D32_SFLOAT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT
                );
            }

            m_ShadowMapSamplers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
            for (auto& sampler : m_ShadowMapSamplers)
            {
                VkSamplerCreateInfo sampler_info{};
                sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                sampler_info.magFilter = VK_FILTER_LINEAR;
                sampler_info.minFilter = VK_FILTER_LINEAR;
                sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                sampler_info.anisotropyEnable = VK_TRUE;
                sampler_info.mipLodBias = 0.0f;
                sampler_info.maxAnisotropy = 1.0f;
                sampler_info.minLod = 0.0f;
                sampler_info.maxLod = 1.0f;
                sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(VulkanContext::GetPhysicalDevice(), &properties);

                sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
                sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                sampler_info.unnormalizedCoordinates = VK_FALSE;
                sampler_info.compareEnable = VK_FALSE;
                sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
                sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                sampler_info.mipLodBias = 0.0f;
                sampler_info.minLod = 0.0f;
                sampler_info.maxLod = 1.0f;

                VK_CHECK_RESULT(vkCreateSampler(device, &sampler_info, nullptr, &sampler));
            }

            // Create Render Pass
            VkAttachmentDescription depthAttachment{};

            // Depth attachment (shadow map)
            depthAttachment.format = VK_FORMAT_D32_SFLOAT;
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            depthAttachment.flags = 0;

            // Attachment references from subpasses
            VkAttachmentReference depth_ref{};
            depth_ref.attachment = 0;
            depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            // Subpass 0: shadow map rendering
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.flags = 0;
            subpass.inputAttachmentCount = 0;
            subpass.pInputAttachments = NULL;
            subpass.colorAttachmentCount = 0;
            subpass.pColorAttachments = NULL;
            subpass.pResolveAttachments = NULL;
            subpass.pDepthStencilAttachment = &depth_ref;
            subpass.preserveAttachmentCount = 0;
            subpass.pPreserveAttachments = NULL;

            // Use subpass dependencies for layout transitions
            std::array<VkSubpassDependency, 2> dependencies;

            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            // Create render pass
            VkRenderPassCreateInfo vk_shadow_map_render_pass_create_info{};
            vk_shadow_map_render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            vk_shadow_map_render_pass_create_info.pNext = NULL;
            vk_shadow_map_render_pass_create_info.attachmentCount = 1;
            vk_shadow_map_render_pass_create_info.pAttachments = &depthAttachment;
            vk_shadow_map_render_pass_create_info.subpassCount = 1;
            vk_shadow_map_render_pass_create_info.pSubpasses = &subpass;
            vk_shadow_map_render_pass_create_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
            vk_shadow_map_render_pass_create_info.pDependencies = dependencies.data();
            vk_shadow_map_render_pass_create_info.flags = 0;

            VK_CHECK_RESULT(vkCreateRenderPass(device, &vk_shadow_map_render_pass_create_info, NULL, &m_ShadowMapRenderPass));

            // Create Shadow Map Framebuffer
            m_ShadowMapFramebuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
            for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
            {
                VkImageView imageView = m_ShadowMapImages[i]->GetImageView();

                VkFramebufferCreateInfo vk_shadow_map_framebuffer_create_info{};
                vk_shadow_map_framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                vk_shadow_map_framebuffer_create_info.pNext = NULL;
                vk_shadow_map_framebuffer_create_info.renderPass = m_ShadowMapRenderPass;
                vk_shadow_map_framebuffer_create_info.attachmentCount = 1;
                vk_shadow_map_framebuffer_create_info.pAttachments = &imageView;
                vk_shadow_map_framebuffer_create_info.width = SHADOW_MAP_WIDTH;
                vk_shadow_map_framebuffer_create_info.height = SHADOW_MAP_HEIGHT;
                vk_shadow_map_framebuffer_create_info.layers = 1;
                vk_shadow_map_framebuffer_create_info.flags = 0;

                VK_CHECK_RESULT(vkCreateFramebuffer(device, &vk_shadow_map_framebuffer_create_info, NULL, &m_ShadowMapFramebuffers[i]));
            }

            // Create Descriptors
            m_ShadowMapUniformBuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
            for (auto& uniformBuffer : m_ShadowMapUniformBuffers)
            {
                uniformBuffer = std::make_unique<VulkanBuffer>(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                uniformBuffer->MapMemory(sizeof(glm::mat4));
            }

            // Creating Descriptors
            VkDescriptorSetLayoutBinding vk_uniform_buffer_object_layout_binding{};
            vk_uniform_buffer_object_layout_binding.binding = 0;
            vk_uniform_buffer_object_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            vk_uniform_buffer_object_layout_binding.descriptorCount = 1;
            vk_uniform_buffer_object_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            vk_uniform_buffer_object_layout_binding.pImmutableSamplers = nullptr;

            std::vector<VkDescriptorSetLayoutBinding> bindings = { vk_uniform_buffer_object_layout_binding };

            m_ShadowMapDescriptorLayout = std::make_unique<VulkanDescriptorLayout>(bindings);
            m_ShadowMapDescriptorWriter = std::make_unique<VulkanDescriptorWriter>(*m_ShadowMapDescriptorLayout);

            m_ShadowMapDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

            for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
            {
                VkDescriptorBufferInfo vk_descriptor_buffer_info{};
                vk_descriptor_buffer_info.buffer = m_ShadowMapUniformBuffers[i]->GetBuffer();
                vk_descriptor_buffer_info.offset = 0;
                vk_descriptor_buffer_info.range = sizeof(glm::mat4);

                VulkanContext::GetCurrentGlobalDescriptorPool()->AllocateDescriptorSet(&m_ShadowMapDescriptorSets[i], m_ShadowMapDescriptorLayout->GetLayout());
                m_ShadowMapDescriptorWriter->WriteBuffer(0, &vk_descriptor_buffer_info);
                m_ShadowMapDescriptorWriter->Update(m_ShadowMapDescriptorSets[i]);
            }

            VkPushConstantRange vk_push_constant_range{};
            vk_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            vk_push_constant_range.size = sizeof(ModelMatrixPushConstantData);
            vk_push_constant_range.offset = 0;

            VkDescriptorSetLayout descriptorSetLayouts[] = { m_ShadowMapDescriptorLayout->GetLayout() };
            VkPushConstantRange pushConstantRanges[] = { vk_push_constant_range };

            VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
            vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            vk_pipeline_layout_create_info.setLayoutCount = 1;
            vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;
            vk_pipeline_layout_create_info.pushConstantRangeCount = 1;
            vk_pipeline_layout_create_info.pPushConstantRanges = pushConstantRanges;

            VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_ShadowMapPipelineLayout));

            VulkanPipelineSpecification pipelineSpec{};
            pipelineSpec.vertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/shadow_mapVert.spv";
            pipelineSpec.fragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/shadow_mapFrag.spv";
            pipelineSpec.subPass = 0;

            pipelineSpec.pipelineLayout = m_ShadowMapPipelineLayout;
            pipelineSpec.renderPass = m_ShadowMapRenderPass;

            VkVertexInputBindingDescription vk_vertex_input_binding_description{};
            vk_vertex_input_binding_description.binding = 0;
            vk_vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            vk_vertex_input_binding_description.stride = sizeof(VulkanVertex);

            const auto& vk_attribute_descriptions = VertexInputAttributeLayout::CreateVertexInputAttributeDescriptions(
                {
                    VertexInputAttribute::VEC3F
                }
            );

            VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info{};
            vk_pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vk_pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
            vk_pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vk_vertex_input_binding_description;
            vk_pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vk_attribute_descriptions.size());
            vk_pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vk_attribute_descriptions.data();

            pipelineSpec.pipelineVertexInputStateCreateInfo = vk_pipeline_vertex_input_state_create_info;

            VulkanPipeline::FillWithDefaultPipelineSpecification(pipelineSpec);

            m_ShadowMapPipeline = std::make_unique<VulkanPipeline>(pipelineSpec);
        }
    }

    void VulkanRenderer::BeginShadowRenderPass(const glm::mat4& lightViewProjectionMatrix)
    {
        VkClearValue clear_value{};
        clear_value.depthStencil.depth = 1.0f;
        clear_value.depthStencil.stencil = 0;

        VkRenderPassBeginInfo vk_shadow_map_render_pass_begin_info{};
        vk_shadow_map_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        vk_shadow_map_render_pass_begin_info.pNext = NULL;
        vk_shadow_map_render_pass_begin_info.renderPass = m_ShadowMapRenderPass;
        vk_shadow_map_render_pass_begin_info.framebuffer = m_ShadowMapFramebuffers[m_CurrentFrame];
        vk_shadow_map_render_pass_begin_info.renderArea.offset.x = 0;
        vk_shadow_map_render_pass_begin_info.renderArea.offset.y = 0;
        vk_shadow_map_render_pass_begin_info.renderArea.extent.width = SHADOW_MAP_WIDTH;
        vk_shadow_map_render_pass_begin_info.renderArea.extent.height = SHADOW_MAP_HEIGHT;
        vk_shadow_map_render_pass_begin_info.clearValueCount = 1;
        vk_shadow_map_render_pass_begin_info.pClearValues = &clear_value;

        const auto& device = VulkanContext::GetCurrentDevice();
        vkCmdBeginRenderPass(device->GetCommandBuffer(m_CurrentFrame), &vk_shadow_map_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        VulkanRenderCommand::SetViewport(device->GetCommandBuffer(m_CurrentFrame), 0.0f, 0.0f, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
        VulkanRenderCommand::SetScissor(device->GetCommandBuffer(m_CurrentFrame), { 0, 0 }, { SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT });

        m_ShadowMapPipeline->Bind(device->GetCommandBuffer(m_CurrentFrame));

        m_ShadowMapUniformBuffers[m_CurrentFrame]->WriteToBuffer(glm::value_ptr(lightViewProjectionMatrix), sizeof(glm::mat4), 0);

        vkCmdBindDescriptorSets(device->GetCommandBuffer(m_CurrentFrame), VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowMapPipelineLayout, 0, 1, &m_ShadowMapDescriptorSets[m_CurrentFrame], 0, nullptr);
    }

    void VulkanRenderer::EndShadowRenderPass()
    {
        const auto& device = VulkanContext::GetCurrentDevice();
        vkCmdEndRenderPass(device->GetCommandBuffer(m_CurrentFrame));
    }

    void VulkanRenderer::CreateViewportResources()
    {
        VkFormat vk_swap_chain_image_format = m_SwapChain->GetSwapChainImageFormat();
        VkSampleCountFlagBits sampleCount = VulkanRenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());

        m_ViewportImagesMSAA.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (auto& viewportImage : m_ViewportImagesMSAA) {
            viewportImage = std::make_shared<VulkanImage>(
                m_ViewportSize.x,
                m_ViewportSize.y,
                1,
                vk_swap_chain_image_format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                sampleCount
            );
        }

        m_ViewportImages.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (auto& viewportImage : m_ViewportImages) {
            viewportImage = std::make_shared<VulkanImage>(
                m_ViewportSize.x,
                m_ViewportSize.y,
                1,
                vk_swap_chain_image_format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );
        }

        VkFormat depthFormat = VulkanSwapChain::GetDepthFormat();
        m_ViewportDepthImage = VulkanImage::Create(
            m_ViewportSize.x,
            m_ViewportSize.y,
            1,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            sampleCount
        );

        // Create Framebuffers
        m_ViewportFramebuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        for (int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
            std::vector<VkImageView> attachments = { m_ViewportImagesMSAA[i]->GetImageView(), m_ViewportDepthImage->GetImageView(), m_ViewportImages[i]->GetImageView() };

            VkFramebufferCreateInfo vk_framebuffer_create_info{};
            vk_framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            vk_framebuffer_create_info.renderPass = m_ViewportRenderPass;
            vk_framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
            vk_framebuffer_create_info.pAttachments = attachments.data();
            vk_framebuffer_create_info.width = m_ViewportSize.x;
            vk_framebuffer_create_info.height = m_ViewportSize.y;
            vk_framebuffer_create_info.layers = 1;

            VK_CHECK_RESULT(vkCreateFramebuffer(device, &vk_framebuffer_create_info, nullptr, &m_ViewportFramebuffers[i]));
        }
    }

    void VulkanRenderer::CreateViewportRenderPass()
    {
        VkFormat vk_swap_chain_image_format = m_SwapChain->GetSwapChainImageFormat();
        VkSampleCountFlagBits sampleCount = VulkanRenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());

        // Subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkSubpassDependency vk_subpass_dependency{};
        vk_subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        vk_subpass_dependency.dstSubpass = 0;
        vk_subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        vk_subpass_dependency.srcAccessMask = 0;
        vk_subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        vk_subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkAttachmentDescription vk_color_attachment_description{};
        vk_color_attachment_description.format = vk_swap_chain_image_format;
        vk_color_attachment_description.samples = sampleCount;
        vk_color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        vk_color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        vk_color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        vk_color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vk_color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // vk_color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        // vk_color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // For ImGui
        vk_color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // For ImGui

        VkAttachmentReference vk_color_attachment_reference{};
        vk_color_attachment_reference.attachment = 0;
        vk_color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription vk_depth_attachment_desc{};
        vk_depth_attachment_desc.format = VulkanSwapChain::GetDepthFormat();
        vk_depth_attachment_desc.samples = sampleCount;
        vk_depth_attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        vk_depth_attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vk_depth_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        vk_depth_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vk_depth_attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vk_depth_attachment_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference vk_depth_attachment_ref{};
        vk_depth_attachment_ref.attachment = 1;
        vk_depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription vk_color_attachment_resolve{};
        vk_color_attachment_resolve.format = vk_swap_chain_image_format;
        vk_color_attachment_resolve.samples = VK_SAMPLE_COUNT_1_BIT;
        vk_color_attachment_resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        vk_color_attachment_resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        vk_color_attachment_resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        vk_color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vk_color_attachment_resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vk_color_attachment_resolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference vk_color_attachment_resolve_ref{};
        vk_color_attachment_resolve_ref.attachment = 2;
        vk_color_attachment_resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        std::array<VkAttachmentDescription, 3> attachments = { vk_color_attachment_description, vk_depth_attachment_desc, vk_color_attachment_resolve };

        VkSubpassDescription vk_subpass_description{};
        vk_subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        vk_subpass_description.colorAttachmentCount = 1;
        vk_subpass_description.pColorAttachments = &vk_color_attachment_reference;
        vk_subpass_description.pDepthStencilAttachment = &vk_depth_attachment_ref;
        vk_subpass_description.pResolveAttachments = &vk_color_attachment_resolve_ref;

        VkRenderPassCreateInfo vk_render_pass_create_info{};
        vk_render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        vk_render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        vk_render_pass_create_info.pAttachments = attachments.data();
        vk_render_pass_create_info.subpassCount = 1;
        vk_render_pass_create_info.pSubpasses = &vk_subpass_description;
        vk_render_pass_create_info.dependencyCount = (uint32_t)dependencies.size();
        vk_render_pass_create_info.pDependencies = dependencies.data();

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VK_CHECK_RESULT(vkCreateRenderPass(device, &vk_render_pass_create_info, nullptr, &m_ViewportRenderPass));
    }

    void VulkanRenderer::InvalidateViewportResources()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        VulkanContext::GetCurrentDevice()->WaitIdle();
        for (auto& framebuffer : m_ViewportFramebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);

        for (auto& image : m_ViewportImages)
            image = nullptr;

        for (auto& image : m_ViewportImagesMSAA)
            image = nullptr;

        CreateViewportResources();
    }

    void VulkanRenderer::UpdateViewportSize(const glm::vec2& viewportSize)
    {
        if (m_ViewportSize != viewportSize && viewportSize.x && viewportSize.y)
        {
            m_ViewportSize = viewportSize;
            InvalidateViewportResources();
        }
    }

    void VulkanRenderer::CreateMousePickingResources()
    {
        m_MousePickingImage = std::make_shared<VulkanImage>(
            m_ViewportSize.x,
            m_ViewportSize.y,
            1,
            VK_FORMAT_R32_SINT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        );

        VkFormat depthFormat = VulkanSwapChain::GetDepthFormat();
        m_MousePickingDepthImage = VulkanImage::Create(
            m_ViewportSize.x,
            m_ViewportSize.y,
            1,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            VK_SAMPLE_COUNT_1_BIT
        );

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        std::vector<VkImageView> attachments = { m_MousePickingImage->GetImageView(), m_MousePickingDepthImage->GetImageView() };

        VkFramebufferCreateInfo vk_framebuffer_create_info{};
        vk_framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        vk_framebuffer_create_info.renderPass = m_MousePickingRenderPass;
        vk_framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        vk_framebuffer_create_info.pAttachments = attachments.data();
        vk_framebuffer_create_info.width = m_ViewportSize.x;
        vk_framebuffer_create_info.height = m_ViewportSize.y;
        vk_framebuffer_create_info.layers = 1;

        VK_CHECK_RESULT(vkCreateFramebuffer(device, &vk_framebuffer_create_info, nullptr, &m_MousePickingFramebuffer));

        // Create Descriptors
        m_MousePickingUniformBuffer = std::make_unique<VulkanBuffer>(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_MousePickingUniformBuffer->MapMemory(sizeof(glm::mat4));

        // Creating Descriptors
        VkDescriptorSetLayoutBinding vk_uniform_buffer_object_layout_binding{};
        vk_uniform_buffer_object_layout_binding.binding = 0;
        vk_uniform_buffer_object_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        vk_uniform_buffer_object_layout_binding.descriptorCount = 1;
        vk_uniform_buffer_object_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        vk_uniform_buffer_object_layout_binding.pImmutableSamplers = nullptr;

        std::vector<VkDescriptorSetLayoutBinding> bindings = { vk_uniform_buffer_object_layout_binding };

        m_MousePickingDescriptorLayout = std::make_unique<VulkanDescriptorLayout>(bindings);
        m_MousePickingDescriptorWriter = std::make_unique<VulkanDescriptorWriter>(*m_MousePickingDescriptorLayout);

        VkDescriptorBufferInfo vk_descriptor_buffer_info{};
        vk_descriptor_buffer_info.buffer = m_MousePickingUniformBuffer->GetBuffer();
        vk_descriptor_buffer_info.offset = 0;
        vk_descriptor_buffer_info.range = sizeof(glm::mat4);

        VulkanContext::GetCurrentGlobalDescriptorPool()->AllocateDescriptorSet(&m_MousePickingDescriptorSet, m_MousePickingDescriptorLayout->GetLayout());
        m_MousePickingDescriptorWriter->WriteBuffer(0, &vk_descriptor_buffer_info);
        m_MousePickingDescriptorWriter->Update(m_MousePickingDescriptorSet);

        // Pipeline Creation
        VkPushConstantRange vk_push_constant_range{};
        vk_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        vk_push_constant_range.size = sizeof(MousePickingPushConstantData);
        vk_push_constant_range.offset = 0;

        VkDescriptorSetLayout descriptorSetLayouts[] = { m_MousePickingDescriptorLayout->GetLayout() };
        VkPushConstantRange pushConstantRanges[] = { vk_push_constant_range };

        VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
        vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vk_pipeline_layout_create_info.setLayoutCount = 1;
        vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;
        vk_pipeline_layout_create_info.pushConstantRangeCount = 1;
        vk_pipeline_layout_create_info.pPushConstantRanges = pushConstantRanges;

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_MousePickingPipelineLayout));

        VulkanPipelineSpecification pipelineSpec{};
        pipelineSpec.vertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/mouse_pickingVert.spv";
        pipelineSpec.fragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/mouse_pickingFrag.spv";
        pipelineSpec.subPass = 0;

        pipelineSpec.pipelineLayout = m_MousePickingPipelineLayout;
        pipelineSpec.renderPass = m_MousePickingRenderPass;

        VkVertexInputBindingDescription vk_vertex_input_binding_description{};
        vk_vertex_input_binding_description.binding = 0;
        vk_vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vk_vertex_input_binding_description.stride = sizeof(VulkanVertex);

        const auto& vk_attribute_descriptions = VertexInputAttributeLayout::CreateVertexInputAttributeDescriptions(
            {
                VertexInputAttribute::VEC3F
            }
        );

        VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info{};
        vk_pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vk_pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        vk_pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vk_vertex_input_binding_description;
        vk_pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vk_attribute_descriptions.size());
        vk_pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vk_attribute_descriptions.data();

        pipelineSpec.pipelineVertexInputStateCreateInfo = vk_pipeline_vertex_input_state_create_info;
        pipelineSpec.blendingEnable = false;

        VulkanPipeline::FillWithDefaultPipelineSpecification(pipelineSpec);

        m_MousePickingPipeline = std::make_unique<VulkanPipeline>(pipelineSpec);
    }

    void VulkanRenderer::CreateMousePickingRenderPass()
    {
        // Subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkSubpassDependency vk_subpass_dependency{};
        vk_subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        vk_subpass_dependency.dstSubpass = 0;
        vk_subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        vk_subpass_dependency.srcAccessMask = 0;
        vk_subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        vk_subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkAttachmentDescription vk_color_attachment_description{};
        vk_color_attachment_description.format = VK_FORMAT_R32_SINT;
        vk_color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
        vk_color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        vk_color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        vk_color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        vk_color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vk_color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vk_color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference vk_color_attachment_reference{};
        vk_color_attachment_reference.attachment = 0;
        vk_color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription vk_depth_attachment_desc{};
        vk_depth_attachment_desc.format = VulkanSwapChain::GetDepthFormat();
        vk_depth_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
        vk_depth_attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        vk_depth_attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vk_depth_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        vk_depth_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vk_depth_attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vk_depth_attachment_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference vk_depth_attachment_ref{};
        vk_depth_attachment_ref.attachment = 1;
        vk_depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::array<VkAttachmentDescription, 2> attachments = { vk_color_attachment_description, vk_depth_attachment_desc };

        VkSubpassDescription vk_subpass_description{};
        vk_subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        vk_subpass_description.colorAttachmentCount = 1;
        vk_subpass_description.pColorAttachments = &vk_color_attachment_reference;
        vk_subpass_description.pDepthStencilAttachment = &vk_depth_attachment_ref;

        VkRenderPassCreateInfo mouse_picking_render_pass_info{};
        mouse_picking_render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        mouse_picking_render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        mouse_picking_render_pass_info.pAttachments = attachments.data();
        mouse_picking_render_pass_info.subpassCount = 1;
        mouse_picking_render_pass_info.pSubpasses = &vk_subpass_description;
        mouse_picking_render_pass_info.dependencyCount = (uint32_t)dependencies.size();
        mouse_picking_render_pass_info.pDependencies = dependencies.data();

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VK_CHECK_RESULT(vkCreateRenderPass(device, &mouse_picking_render_pass_info, nullptr, &m_MousePickingRenderPass));
    }

    void VulkanRenderer::WriteMousePickingImagePixelToBuffer(VkBuffer buffer, const glm::vec2& pixelOffset)
    {
        const auto& device = VulkanContext::GetCurrentDevice();

        VkCommandBuffer commandBuffer;
        device->BeginSingleTimeCommandBuffer(commandBuffer);
        {
            VkPipelineStageFlags sourceStageFlags;
            VkPipelineStageFlags destinationStageFlags;

            VkImageMemoryBarrier vk_image_memory_barrier{};

            sourceStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

            vk_image_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vk_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            vk_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            vk_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            vk_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_image_memory_barrier.image = m_MousePickingImage->GetImage();
            vk_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vk_image_memory_barrier.subresourceRange.baseMipLevel = 0;
            vk_image_memory_barrier.subresourceRange.levelCount = 1;
            vk_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
            vk_image_memory_barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(commandBuffer, sourceStageFlags, destinationStageFlags, 0, 0, nullptr, 0, nullptr, 1, &vk_image_memory_barrier);
        }

        VkBufferImageCopy region{};
        region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.imageOffset = { (int)pixelOffset.x, (int)pixelOffset.y };
        region.imageExtent = { 1, 1, 1 };

        region.bufferOffset = 0;
        region.bufferRowLength = 1;
        region.bufferImageHeight = 1;

        vkCmdCopyImageToBuffer(commandBuffer, m_MousePickingImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

        {
            VkPipelineStageFlags sourceStageFlags;
            VkPipelineStageFlags destinationStageFlags;

            VkImageMemoryBarrier vk_image_memory_barrier{};

            sourceStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

            vk_image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

            vk_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            vk_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            vk_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            vk_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_image_memory_barrier.image = m_MousePickingImage->GetImage();
            vk_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vk_image_memory_barrier.subresourceRange.baseMipLevel = 0;
            vk_image_memory_barrier.subresourceRange.levelCount = 1;
            vk_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
            vk_image_memory_barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(commandBuffer, sourceStageFlags, destinationStageFlags, 0, 0, nullptr, 0, nullptr, 1, &vk_image_memory_barrier);
        }
        device->EndSingleTimeCommandBuffer(commandBuffer);
    }

    void VulkanRenderer::BeginMousePickingRenderPass(const glm::vec2& renderOffset, const glm::mat4& viewProjectionMatrix)
    {
        VkRenderPassBeginInfo vk_render_pass_begin_info{};
        vk_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        vk_render_pass_begin_info.renderPass = m_MousePickingRenderPass;
        vk_render_pass_begin_info.framebuffer = m_MousePickingFramebuffer;
        vk_render_pass_begin_info.renderArea.offset = { (int32_t)renderOffset.x, (int32_t)renderOffset.y };
        vk_render_pass_begin_info.renderArea.extent = VkExtent2D{ 1, 1 };

        std::array<VkClearValue, 2> vk_clear_values{};
        vk_clear_values[0].color = { .int32 = {-1} };
        vk_clear_values[1].depthStencil = { 1.0f, 0 };

        vk_render_pass_begin_info.clearValueCount = static_cast<uint32_t>(vk_clear_values.size());
        vk_render_pass_begin_info.pClearValues = vk_clear_values.data();

        const auto& device = VulkanContext::GetCurrentDevice();
        vkCmdBeginRenderPass(device->GetCommandBuffer(m_CurrentFrame), &vk_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        m_MousePickingPipeline->Bind(device->GetCommandBuffer(m_CurrentFrame));

        m_MousePickingUniformBuffer->WriteToBuffer(glm::value_ptr(viewProjectionMatrix), sizeof(glm::mat4), 0);

        vkCmdBindDescriptorSets(device->GetCommandBuffer(m_CurrentFrame), VK_PIPELINE_BIND_POINT_GRAPHICS, m_MousePickingPipelineLayout, 0, 1, &m_MousePickingDescriptorSet, 0, nullptr);
    }

    void VulkanRenderer::EndMousePickingRenderPass()
    {
        const auto& device = VulkanContext::GetCurrentDevice();
        vkCmdEndRenderPass(device->GetCommandBuffer(m_CurrentFrame));
    }

    VkCommandBuffer VulkanRenderer::BeginFrame()
    {
        VkResult result = m_SwapChain->AcquireNextImage();
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_SwapChain->Invalidate();
            return VK_NULL_HANDLE;
        }
        m_ImageIndex = m_SwapChain->GetAcquiredImageIndex();

        const auto& device = VulkanContext::GetCurrentDevice();
        device->ResetCommandBuffer(m_CurrentFrame);
        device->BeginCommandBuffer(m_CurrentFrame);
        return device->GetCommandBuffer(m_CurrentFrame);
    }

    bool VulkanRenderer::EndFrame()
    {
        bool isResized = false;
        const auto& device = VulkanContext::GetCurrentDevice();
        device->EndCommandBuffer(m_CurrentFrame);

        VkResult queuePresentStatus = m_SwapChain->SubmitCommandBuffer(device->GetCommandBuffer(m_CurrentFrame));
        if (queuePresentStatus == VK_ERROR_OUT_OF_DATE_KHR || queuePresentStatus == VK_SUBOPTIMAL_KHR || VulkanContext::GetCurrentWindow()->IsWindowResized())
        {
            m_SwapChain->Invalidate(m_SwapChain->GetVulkanSwapChain());
            VulkanContext::GetCurrentWindow()->ResetWindowResizedFlag();
            isResized = true;
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
        return isResized;
    }

    void VulkanRenderer::BeginViewportRenderPass(const glm::vec3& clearColor)
    {
        VkRenderPassBeginInfo vk_render_pass_begin_info{};
        vk_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        vk_render_pass_begin_info.renderPass = m_ViewportRenderPass;
        vk_render_pass_begin_info.framebuffer = m_ViewportFramebuffers[m_ImageIndex];
        vk_render_pass_begin_info.renderArea.offset = { 0, 0 };
        vk_render_pass_begin_info.renderArea.extent = VkExtent2D{ (uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y };

        std::array<VkClearValue, 2> vk_clear_values{};
        vk_clear_values[0].color = { clearColor.r, clearColor.g, clearColor.b, 1.0f };
        vk_clear_values[1].depthStencil = { 1.0f, 0 };

        vk_render_pass_begin_info.clearValueCount = static_cast<uint32_t>(vk_clear_values.size());
        vk_render_pass_begin_info.pClearValues = vk_clear_values.data();

        const auto& device = VulkanContext::GetCurrentDevice();
        vkCmdBeginRenderPass(device->GetCommandBuffer(m_CurrentFrame), &vk_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderer::EndViewportRenderPass()
    {
        const auto& device = VulkanContext::GetCurrentDevice();
        vkCmdEndRenderPass(device->GetCommandBuffer(m_CurrentFrame));
    }

    VulkanRenderer::~VulkanRenderer()
    {
        VulkanContext::GetCurrentDevice()->WaitIdle();

        VulkanTexture::DestroyStaticResources();

        AssetManager::DestroyAssets();

        // Destroy Shadow Map Resources
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyPipelineLayout(device, m_ShadowMapPipelineLayout, nullptr);

        for (auto& framebuffer : m_ShadowMapFramebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);

        vkDestroyRenderPass(device, m_ShadowMapRenderPass, nullptr);

        for (auto& sampler : m_ShadowMapSamplers)
            vkDestroySampler(device, sampler, nullptr);

        // Destroy Viewport Resources

        for (auto& framebuffer : m_ViewportFramebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);

        vkDestroyRenderPass(device, m_ViewportRenderPass, nullptr);

        // Destroy Mouse Picking Resources
        vkDestroyPipelineLayout(device, m_MousePickingPipelineLayout, nullptr);
        vkDestroyFramebuffer(device, m_MousePickingFramebuffer, nullptr);
        vkDestroyRenderPass(device, m_MousePickingRenderPass, nullptr);
    }
}
