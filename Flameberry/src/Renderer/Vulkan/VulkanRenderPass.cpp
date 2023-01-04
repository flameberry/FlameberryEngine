// #include "VulkanRenderPass.h"

// #include "VulkanDebug.h"
// #include "VulkanContext.h"

// namespace Flameberry {
//     VulkanRenderPass::VulkanRenderPass()
//     {
//         VkAttachmentDescription depthAttachment{};

//         // Depth attachment (shadow map)
//         depthAttachment.format = VK_FORMAT_D32_SFLOAT;
//         depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
//         depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//         depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//         depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//         depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//         depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//         depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
//         depthAttachment.flags = 0;

//         // Attachment references from subpasses
//         VkAttachmentReference depth_ref{};
//         depth_ref.attachment = 0;
//         depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

//         // Subpass 0: shadow map rendering
//         VkSubpassDescription subpass{};
//         subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//         subpass.flags = 0;
//         subpass.inputAttachmentCount = 0;
//         subpass.pInputAttachments = NULL;
//         subpass.colorAttachmentCount = 0;
//         subpass.pColorAttachments = NULL;
//         subpass.pResolveAttachments = NULL;
//         subpass.pDepthStencilAttachment = &depth_ref;
//         subpass.preserveAttachmentCount = 0;
//         subpass.pPreserveAttachments = NULL;

//         // Use subpass dependencies for layout transitions
//         std::array<VkSubpassDependency, 2> dependencies;

//         dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
//         dependencies[0].dstSubpass = 0;
//         dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//         dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//         dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
//         dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//         dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

//         dependencies[1].srcSubpass = 0;
//         dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
//         dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
//         dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//         dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//         dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//         dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

//         // Create render pass
//         VkRenderPassCreateInfo vk_render_pass_create_info{};
//         vk_render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//         vk_render_pass_create_info.pNext = NULL;
//         vk_render_pass_create_info.attachmentCount = 1;
//         vk_render_pass_create_info.pAttachments = &depthAttachment;
//         vk_render_pass_create_info.subpassCount = 1;
//         vk_render_pass_create_info.pSubpasses = &subpass;
//         vk_render_pass_create_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
//         vk_render_pass_create_info.pDependencies = dependencies.data();
//         vk_render_pass_create_info.flags = 0;

//         const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
//         VK_CHECK_RESULT(vkCreateRenderPass(device, &vk_render_pass_create_info, NULL, &m_VkRenderPass));
//     }

//     VulkanRenderPass::~VulkanRenderPass()
//     {
//     }
// }