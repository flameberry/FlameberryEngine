#include "SkyboxRenderer.h"

// #include <glad/glad.h>
// #include <glm/gtc/type_ptr.hpp>

// #include "Vulkan/VulkanContext.h"
// #include "Vulkan/VulkanDebug.h"
// #include "Vulkan/VulkanRenderCommand.h"
// #include "Vulkan/VulkanVertex.h"

// namespace Flameberry {
//     SkyboxRenderer::SkyboxRenderer(const std::shared_ptr<VulkanDescriptorPool>& globalDescriptorPool, VkDescriptorSetLayout globalDescriptorLayout, VkRenderPass renderPass)
//         : m_GlobalDescriptorPool(globalDescriptorPool), m_CubeMesh(FL_PROJECT_DIR"SandboxApp/assets/models/cube.obj")
//     {
//         const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
//         VkDescriptorSetLayout descriptorSetLayouts[] = { globalDescriptorLayout };

//         VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
//         vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//         vk_pipeline_layout_create_info.setLayoutCount = sizeof(descriptorSetLayouts) / sizeof(VkDescriptorSetLayout);
//         vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;

//         VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_SkyboxPipelineLayout));

//         Flameberry::PipelineSpecification pipelineSpec{};
//         pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/skyboxVert.spv";
//         pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/skyboxFrag.spv";
//         pipelineSpec.renderPass = renderPass;
//         pipelineSpec.SubPass = 0;

//         pipelineSpec.PipelineLayout = m_SkyboxPipelineLayout;

//         VkVertexInputBindingDescription vk_vertex_input_binding_description{};
//         vk_vertex_input_binding_description.binding = 0;
//         vk_vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//         vk_vertex_input_binding_description.stride = sizeof(VulkanVertex);

//         const auto& vk_attribute_descriptions = VertexInputAttributeLayout::CreateVertexInputAttributeDescriptions(
//             {
//                 VertexInputAttribute::VEC3F
//             }
//         );

//         VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info{};
//         vk_pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//         vk_pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
//         vk_pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vk_vertex_input_binding_description;
//         vk_pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vk_attribute_descriptions.size());
//         vk_pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vk_attribute_descriptions.data();

//         pipelineSpec.pipelineVertexInputStateCreateInfo = vk_pipeline_vertex_input_state_create_info;

//         Flameberry::Pipeline::FillWithDefaultPipelineSpecification(pipelineSpec);

//         VkSampleCountFlagBits sampleCount = VulkanRenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());

//         pipelineSpec.pipelineMultisampleStateCreateInfo.rasterizationSamples = sampleCount;
//         pipelineSpec.pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_TRUE;

//         // pipelineSpec.pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;

//         pipelineSpec.pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
//         pipelineSpec.pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;

//         m_SkyboxPipeline = std::make_unique<Flameberry::Pipeline>(pipelineSpec);

//         // {
//         //     float skyboxVertices[] = {
//         //         //   Coordinates
//         //         -1.0f, -1.0f,  1.0f,//        7--------6
//         //          1.0f, -1.0f,  1.0f,//       /|       /|
//         //          1.0f, -1.0f, -1.0f,//      4--------5 |
//         //         -1.0f, -1.0f, -1.0f,//      | |      | |
//         //         -1.0f,  1.0f,  1.0f,//      | 3------|-2
//         //          1.0f,  1.0f,  1.0f,//      |/       |/
//         //          1.0f,  1.0f, -1.0f,//      0--------1
//         //         -1.0f,  1.0f, -1.0f
//         //     };

//         //     // Creating Vertex Buffer
//         //     VkDeviceSize bufferSize = sizeof(skyboxVertices);
//         //     VulkanBuffer stagingBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

//         //     stagingBuffer.MapMemory(bufferSize);
//         //     stagingBuffer.WriteToBuffer(skyboxVertices, bufferSize, 0);
//         //     stagingBuffer.UnmapMemory();

//         //     m_VertexBuffer = std::make_unique<VulkanBuffer>(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//         //     VulkanRenderCommand::CopyBuffer(stagingBuffer.GetBuffer(), m_VertexBuffer->GetBuffer(), bufferSize);
//         // }

//         // {
//         //     uint32_t skyboxIndices[] = {
//         //         // Right
//         //         1, 2, 6,
//         //         6, 5, 1,
//         //         // Left
//         //         0, 4, 7,
//         //         7, 3, 0,
//         //         // Top
//         //         4, 5, 6,
//         //         6, 7, 4,
//         //         // Bottom
//         //         0, 3, 2,
//         //         2, 1, 0,
//         //         // Back
//         //         0, 1, 5,
//         //         5, 4, 0,
//         //         // Front
//         //         3, 7, 6,
//         //         6, 2, 3
//         //     };

//         //     // Creating Index Buffer
//         //     VkDeviceSize bufferSize = sizeof(skyboxIndices);
//         //     VulkanBuffer stagingBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

//         //     stagingBuffer.MapMemory(bufferSize);
//         //     stagingBuffer.WriteToBuffer(skyboxIndices, bufferSize, 0);
//         //     stagingBuffer.UnmapMemory();

//         //     m_IndexBuffer = std::make_unique<VulkanBuffer>(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//         //     VulkanRenderCommand::CopyBuffer(stagingBuffer.GetBuffer(), m_IndexBuffer->GetBuffer(), bufferSize);
//         // }
//     }


//     // SkyboxRenderer::SkyboxRenderer(const char* folderPath)
//     //     : m_FolderPath(folderPath), m_VertexArrayID(0), m_VertexBufferID(0), m_IndexBufferID(0), m_ShaderProgramID(0)
//     // {
//     //     Load(folderPath);
//     // }

//     void SkyboxRenderer::Load(const char* folderPath)
//     {
//         float skyboxVertices[] = {
//             //   Coordinates
//             -1.0f, -1.0f,  1.0f,//        7--------6
//              1.0f, -1.0f,  1.0f,//       /|       /|
//              1.0f, -1.0f, -1.0f,//      4--------5 |
//             -1.0f, -1.0f, -1.0f,//      | |      | |
//             -1.0f,  1.0f,  1.0f,//      | 3------|-2
//              1.0f,  1.0f,  1.0f,//      |/       |/
//              1.0f,  1.0f, -1.0f,//      0--------1
//             -1.0f,  1.0f, -1.0f
//         };

//         unsigned int skyboxIndices[] = {
//             // Right
//             1, 2, 6,
//             6, 5, 1,
//             // Left
//             0, 4, 7,
//             7, 3, 0,
//             // Top
//             4, 5, 6,
//             6, 7, 4,
//             // Bottom
//             0, 3, 2,
//             2, 1, 0,
//             // Back
//             0, 1, 5,
//             5, 4, 0,
//             // Front
//             3, 7, 6,
//             6, 2, 3
//         };

//         // m_SkyboxRendererTexture = OpenGLTexture::Create(folderPath);
//     }

//     void SkyboxRenderer::OnDraw(
//         VkCommandBuffer commandBuffer,
//         uint32_t currentFrameIndex,
//         VkDescriptorSet globalDescriptorSet,
//         const PerspectiveCamera& activeCamera,
//         const char* path
//     )
//     {
//         m_SkyboxPipeline->Bind(commandBuffer);

//         vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyboxPipelineLayout, 0, 1, &globalDescriptorSet, 0, nullptr);

//         m_CubeMesh.Bind(commandBuffer);
//         m_CubeMesh.OnDraw(commandBuffer);
//         // VkBuffer vk_vertex_buffers[] = { m_VertexBuffer->GetBuffer() };
//         // VkDeviceSize offsets[] = { 0 };
//         // vkCmdBindVertexBuffers(commandBuffer, 0, 1, vk_vertex_buffers, offsets);
//         // vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

//         // vkCmdDrawIndexed(commandBuffer, 36, 1, 0, 0, 0);
//     }

//     SkyboxRenderer::~SkyboxRenderer()
//     {
//         const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
//         vkDestroyPipelineLayout(device, m_SkyboxPipelineLayout, nullptr);
//     }
// }
