#include "RenderPass.h"

#include <array>

#include "VulkanDebug.h"
#include "RenderCommand.h"
#include "VulkanContext.h"
#include "Renderer/Renderer.h"

namespace Flameberry {
    RenderPass::RenderPass(const RenderPassSpecification& specification)
        : m_RenderPassSpec(specification)
    {
        FBY_ASSERT(m_RenderPassSpec.TargetFramebuffers.size(), "No Target Framebuffers provided to RenderPass!");

        // Checking if all the Framebuffers have same specification
        for (uint32_t i = 1; i < m_RenderPassSpec.TargetFramebuffers.size(); i++)
        {
            const auto& spec = m_RenderPassSpec.TargetFramebuffers[i]->GetSpecification();
            FBY_ASSERT(spec.Attachments == m_RenderPassSpec.TargetFramebuffers[0]->GetSpecification().Attachments, "Framebuffers having different attachments must not be provided to single RenderPass!");
            FBY_ASSERT(spec.Samples == m_RenderPassSpec.TargetFramebuffers[0]->GetSpecification().Samples, "Framebuffers having different sample count must not be provided to single RenderPass!");
        }

        const auto& framebufferSpec = m_RenderPassSpec.TargetFramebuffers[0]->GetSpecification();

        if (!m_RenderPassSpec.Dependencies.size())
        {
            m_RenderPassSpec.Dependencies.resize(2);

            m_RenderPassSpec.Dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            m_RenderPassSpec.Dependencies[0].dstSubpass = 0;
            m_RenderPassSpec.Dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            m_RenderPassSpec.Dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            m_RenderPassSpec.Dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            m_RenderPassSpec.Dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            m_RenderPassSpec.Dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            m_RenderPassSpec.Dependencies[1].srcSubpass = 0;
            m_RenderPassSpec.Dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            m_RenderPassSpec.Dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            m_RenderPassSpec.Dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            m_RenderPassSpec.Dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            m_RenderPassSpec.Dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            m_RenderPassSpec.Dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        }

        // VkSampleCountFlagBits samples = (VkSampleCountFlagBits)m_RenderPassSpec.Samples;
        VkSampleCountFlagBits samples;

        switch (framebufferSpec.Samples)
        {
            case 1:  samples = VK_SAMPLE_COUNT_1_BIT;  break;
            case 2:  samples = VK_SAMPLE_COUNT_2_BIT;  break;
            case 4:  samples = VK_SAMPLE_COUNT_4_BIT;  break;
            case 8:  samples = VK_SAMPLE_COUNT_8_BIT;  break;
            case 16: samples = VK_SAMPLE_COUNT_16_BIT; break;
            case 32: samples = VK_SAMPLE_COUNT_32_BIT; break;
            case 64: samples = VK_SAMPLE_COUNT_64_BIT; break;
            default: samples = VK_SAMPLE_COUNT_1_BIT;  break;
        }

        const auto count = framebufferSpec.Samples > 1 ?
            2 * framebufferSpec.Attachments.size() - 1
            : framebufferSpec.Attachments.size();

        std::vector<VkAttachmentDescription> attachments(count);

        std::vector<VkAttachmentReference> colorRefs;
        VkAttachmentReference depthRef;
        std::vector<VkAttachmentReference> resolveRefs;

        bool useMultiView = false;
        uint32_t layerCount = framebufferSpec.Attachments[0].LayerCount;

        int i = 0;
        for (const auto& attachment : framebufferSpec.Attachments)
        {
            attachments[i].format = attachment.Format;
            attachments[i].samples = samples;
            attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            attachments[i].loadOp = framebufferSpec.DepthLoadOp;
            attachments[i].storeOp = framebufferSpec.DepthStoreOp;

            useMultiView = useMultiView || attachment.LayerCount > 1;
            FBY_ASSERT(layerCount == attachment.LayerCount, "Different Layers for each attachment are not handled correctly yet!");

            if (RenderCommand::DoesFormatSupportDepthAttachment(attachment.Format))
            {
                attachments[i].stencilLoadOp = framebufferSpec.StencilLoadOp;
                attachments[i].stencilStoreOp = framebufferSpec.StencilStoreOp;

                attachments[i].loadOp = framebufferSpec.DepthLoadOp;
                attachments[i].storeOp = framebufferSpec.DepthStoreOp;
                attachments[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

                depthRef.attachment = i;
                depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
            else
            {
                attachments[i].loadOp = framebufferSpec.ColorLoadOp;
                attachments[i].storeOp = framebufferSpec.ColorStoreOp;
                attachments[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                auto& ref = colorRefs.emplace_back();
                ref.attachment = i;
                ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                if (framebufferSpec.Samples > 1)
                {
                    attachments[i + framebufferSpec.Attachments.size()].format = attachment.Format;
                    attachments[i + framebufferSpec.Attachments.size()].samples = VK_SAMPLE_COUNT_1_BIT;
                    attachments[i + framebufferSpec.Attachments.size()].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    attachments[i + framebufferSpec.Attachments.size()].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                    attachments[i + framebufferSpec.Attachments.size()].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    attachments[i + framebufferSpec.Attachments.size()].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    attachments[i + framebufferSpec.Attachments.size()].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    attachments[i + framebufferSpec.Attachments.size()].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    auto& resolveRef = resolveRefs.emplace_back();
                    resolveRef.attachment = i + framebufferSpec.Attachments.size();
                    resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
            }
            i++;
        }

        VkSubpassDescription vk_subpass_description{};
        vk_subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        vk_subpass_description.colorAttachmentCount = colorRefs.size();
        vk_subpass_description.pColorAttachments = colorRefs.data();
        vk_subpass_description.pDepthStencilAttachment = &depthRef;
        vk_subpass_description.pResolveAttachments = resolveRefs.data();

        /*
            Bit mask that specifies which view rendering is broadcast to
            0011 = Broadcast to first and second view (layer)
        */
        uint32_t viewMask = 0b00000000;
        for (uint32_t i = 0; i < layerCount; i++)
            viewMask = (viewMask << 1) + 0b1;

        /*
            Bit mask that specifies correlation between views
            An implementation may use this for optimizations (concurrent render)
        */
        const uint32_t correlationMask = viewMask;

        VkRenderPassMultiviewCreateInfo multiViewRenderPassCreateInfo{};
        multiViewRenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
        multiViewRenderPassCreateInfo.subpassCount = 1;
        multiViewRenderPassCreateInfo.pViewMasks = &viewMask;
        multiViewRenderPassCreateInfo.correlationMaskCount = 1;
        multiViewRenderPassCreateInfo.pCorrelationMasks = &correlationMask;

        VkRenderPassCreateInfo vk_render_pass_create_info{};
        vk_render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        vk_render_pass_create_info.attachmentCount = count;
        vk_render_pass_create_info.pAttachments = attachments.data();
        vk_render_pass_create_info.subpassCount = 1;
        vk_render_pass_create_info.pSubpasses = &vk_subpass_description;
        vk_render_pass_create_info.dependencyCount = (uint32_t)m_RenderPassSpec.Dependencies.size();
        vk_render_pass_create_info.pDependencies = m_RenderPassSpec.Dependencies.data();

        if (useMultiView)
            vk_render_pass_create_info.pNext = &multiViewRenderPassCreateInfo;

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VK_CHECK_RESULT(vkCreateRenderPass(device, &vk_render_pass_create_info, nullptr, &m_VkRenderPass));

        for (auto& framebuffer : m_RenderPassSpec.TargetFramebuffers)
            framebuffer->CreateVulkanFramebuffer(m_VkRenderPass);
    }

    void RenderPass::Begin(uint32_t framebufferInstance, VkOffset2D renderAreaOffset, VkExtent2D renderAreaExtent)
    {
        Renderer::Submit([renderPass = this, framebufferInstance, renderAreaOffset, renderAreaExtent](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                uint32_t index = (framebufferInstance == -1) ? imageIndex : framebufferInstance;
                const auto& framebufferSpec = renderPass->m_RenderPassSpec.TargetFramebuffers[index]->GetSpecification();

                std::vector<VkClearValue> clearValues;
                clearValues.resize(framebufferSpec.Attachments.size());

                for (uint32_t i = 0; i < framebufferSpec.Attachments.size(); i++)
                {
                    auto& value = clearValues[i];
                    if (RenderCommand::DoesFormatSupportDepthAttachment(framebufferSpec.Attachments[i].Format))
                        value.depthStencil = framebufferSpec.DepthStencilClearValue;
                    else
                        value.color = framebufferSpec.ClearColorValue;
                }

                auto framebuffer = renderPass->m_RenderPassSpec.TargetFramebuffers[index]->GetVulkanFramebuffer();

                const auto& renderAreaExt = (renderAreaExtent.width == 0 || renderAreaExtent.height == 0) ?
                    VkExtent2D{ framebufferSpec.Width, framebufferSpec.Height } : renderAreaExtent;

                VkRenderPassBeginInfo vk_render_pass_begin_info{};
                vk_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                vk_render_pass_begin_info.renderPass = renderPass->m_VkRenderPass;
                vk_render_pass_begin_info.framebuffer = framebuffer;

                vk_render_pass_begin_info.renderArea.offset = renderAreaOffset;
                vk_render_pass_begin_info.renderArea.extent = renderAreaExt;

                vk_render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clearValues.size());
                vk_render_pass_begin_info.pClearValues = clearValues.data();

                vkCmdBeginRenderPass(cmdBuffer, &vk_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
            }
        );
    }

    void RenderPass::End()
    {
        Renderer::Submit([](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                vkCmdEndRenderPass(cmdBuffer);
            }
        );
    }

    RenderPass::~RenderPass()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyRenderPass(device, m_VkRenderPass, nullptr);
    }
}
