#include "Renderer2D.h"

#include "Renderer.h"

#include "Vulkan/RenderPass.h"
#include "Vulkan/VulkanDebug.h"
#include "Vulkan/RenderCommand.h"

#define MAX_LINES 500

namespace Flameberry {
    std::shared_ptr<Pipeline> Renderer2D::s_LinePipeline;
    VkPipelineLayout Renderer2D::s_LinePipelineLayout;
    std::shared_ptr<Buffer> Renderer2D::s_LineVertexBuffer;
    std::vector<LineVertex> Renderer2D::s_LineVertices;

    void Renderer2D::Init(VkDescriptorSetLayout globalDescriptorSetLayout, const std::shared_ptr<RenderPass>& renderPass)
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        {
            // Line Resources
            BufferSpecification bufferSpec{};
            bufferSpec.InstanceCount = 1;
            bufferSpec.InstanceSize = MAX_LINES * 2 * sizeof(LineVertex);
            bufferSpec.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            bufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            s_LineVertexBuffer = Buffer::Create(bufferSpec);
            s_LineVertexBuffer->MapMemory(bufferSpec.InstanceSize);

            VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
            vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            vk_pipeline_layout_create_info.setLayoutCount = 1;
            vk_pipeline_layout_create_info.pSetLayouts = &globalDescriptorSetLayout;
            vk_pipeline_layout_create_info.pushConstantRangeCount = 0;
            vk_pipeline_layout_create_info.pPushConstantRanges = nullptr;

            VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &s_LinePipelineLayout));

            PipelineSpecification pipelineSpec{};
            pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/solid_color.vert.spv";
            pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/solid_color.frag.spv";
            pipelineSpec.RenderPass = renderPass;
            pipelineSpec.PipelineLayout = s_LinePipelineLayout;

            pipelineSpec.VertexLayout = {
                VertexInputAttribute::VEC3F, // a_Position
                VertexInputAttribute::VEC3F  // a_Color
            };
            pipelineSpec.VertexInputBindingDescription.binding = 0;
            pipelineSpec.VertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            pipelineSpec.VertexInputBindingDescription.stride = sizeof(LineVertex);
            pipelineSpec.Samples = RenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());

            pipelineSpec.PolygonMode = VK_POLYGON_MODE_LINE;
            pipelineSpec.PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            pipelineSpec.CullMode = VK_CULL_MODE_NONE;

            s_LinePipeline = Pipeline::Create(pipelineSpec);
        }
    }

    void Renderer2D::AddGrid(int gridSize)
    {
        constexpr float squareSize = 1.0f;
        const float length = 2 * gridSize * squareSize;

        const float start = -gridSize * squareSize;

        float current = start;

        constexpr glm::vec3 lineColor(0.35f);
        constexpr glm::vec3 boldLineColor(0.7f);
        // constexpr glm::vec3 redLineColor(0.8f, 0.1f, 0.15f);
        // constexpr glm::vec3 blueLineColor(0.1f, 0.25f, 0.8f);
        // constexpr glm::vec3 redLineColor(1, 0, 0);
        // constexpr glm::vec3 blueLineColor(0, 0, 1);
        constexpr glm::vec3 redLineColor(1.0f, 0.235f, 0.286f);
        constexpr glm::vec3 blueLineColor(0.286f, 0.235f, 1.0f);

        for (int i = -gridSize; i <= gridSize; i++)
        {
            // bool is_bold = i == 0 || i == start || i == -start;
            bool is_bold = i == 0;

            Renderer2D::AddLine({ current, 0.0f, start }, { current, 0.0f, start + length }, is_bold ? blueLineColor : lineColor);
            Renderer2D::AddLine({ start, 0.0f, current }, { start + length, 0.0f, current }, is_bold ? redLineColor : lineColor);
            // Renderer2D::AddLine({ current, 0.0f, start }, { current, 0.0f, start + length }, is_bold ? boldLineColor : lineColor);
            // Renderer2D::AddLine({ start, 0.0f, current }, { start + length, 0.0f, current }, is_bold ? boldLineColor : lineColor);
            current += squareSize;
        }
    }

    void Renderer2D::AddLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color)
    {
        s_LineVertices.push_back(LineVertex{ start, color });
        s_LineVertices.push_back(LineVertex{ end, color });
    }

    void Renderer2D::Render(VkDescriptorSet globalDescriptorSet)
    {
        if (s_LineVertices.size())
        {
            FL_ASSERT(s_LineVertices.size() <= 2 * MAX_LINES, "MAX_LINES limit reached!");
            s_LineVertexBuffer->WriteToBuffer(s_LineVertices.data(), s_LineVertices.size() * sizeof(LineVertex), 0);
            s_LinePipeline->Bind();
            uint32_t vertexCount = s_LineVertices.size();

            auto vertexBuffer = s_LineVertexBuffer->GetBuffer();
            auto pipelineLayout = s_LinePipelineLayout;

            Renderer::Submit([pipelineLayout, globalDescriptorSet, vertexBuffer, vertexCount](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                {
                    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &globalDescriptorSet, 0, nullptr);

                    VkDeviceSize offsets[] = { 0 };
                    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);

                    vkCmdDraw(cmdBuffer, vertexCount, 1, 0, 0);
                }
            );
            s_LineVertices.clear();
        }
    }

    void Renderer2D::Destroy()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyPipelineLayout(device, s_LinePipelineLayout, nullptr);
        s_LinePipeline = nullptr;
        s_LineVertexBuffer = nullptr;
    }
}
