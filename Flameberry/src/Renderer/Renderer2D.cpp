#include "Renderer2D.h"

#include "Renderer.h"

#include "Vulkan/RenderPass.h"
#include "Vulkan/VulkanDebug.h"
#include "Vulkan/RenderCommand.h"

#define MAX_LINES 500
#define MAX_QUADS 100
#define MAX_QUAD_VERTICES MAX_QUADS * 4
#define MAX_QUAD_INDICES MAX_QUADS * 6

namespace Flameberry {
    Renderer2DData Renderer2D::s_Renderer2DData;

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

            s_Renderer2DData.LineVertexBuffer = Buffer::Create(bufferSpec);
            s_Renderer2DData.LineVertexBuffer->MapMemory(bufferSpec.InstanceSize);

            VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
            vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            vk_pipeline_layout_create_info.setLayoutCount = 1;
            vk_pipeline_layout_create_info.pSetLayouts = &globalDescriptorSetLayout;
            vk_pipeline_layout_create_info.pushConstantRangeCount = 0;
            vk_pipeline_layout_create_info.pPushConstantRanges = nullptr;

            VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &s_Renderer2DData.LinePipelineLayout));

            PipelineSpecification pipelineSpec{};
            pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/solid_color.vert.spv";
            pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/solid_color.frag.spv";
            pipelineSpec.RenderPass = renderPass;
            pipelineSpec.PipelineLayout = s_Renderer2DData.LinePipelineLayout;

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

            s_Renderer2DData.LinePipeline = Pipeline::Create(pipelineSpec);
        }

        {
            // Quad Resources
            BufferSpecification bufferSpec{};
            bufferSpec.InstanceCount = 1;
            bufferSpec.InstanceSize = MAX_QUAD_VERTICES * sizeof(LineVertex);
            bufferSpec.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            bufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            s_Renderer2DData.QuadVertexBuffer = Buffer::Create(bufferSpec);
            s_Renderer2DData.QuadVertexBuffer->MapMemory(bufferSpec.InstanceSize);

            BufferSpecification indexBufferSpec{};
            indexBufferSpec.InstanceCount = 1;
            indexBufferSpec.InstanceSize = MAX_QUAD_INDICES * sizeof(uint32_t);
            indexBufferSpec.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            indexBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            s_Renderer2DData.QuadIndexBuffer = Buffer::Create(indexBufferSpec);

            uint32_t* indices = new uint32_t[MAX_QUAD_INDICES];
            uint32_t offset = 0;
            for (uint32_t i = 0; i < MAX_QUAD_INDICES; i += 6)
            {
                indices[i + 0] = offset + 0;
                indices[i + 1] = offset + 1;
                indices[i + 2] = offset + 2;

                indices[i + 3] = offset + 2;
                indices[i + 4] = offset + 3;
                indices[i + 5] = offset + 0;

                offset += 4;
            }

            BufferSpecification stagingBufferSpec{};
            stagingBufferSpec.InstanceCount = 1;
            stagingBufferSpec.InstanceSize = indexBufferSpec.InstanceSize;
            stagingBufferSpec.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            stagingBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            Buffer stagingBuffer(stagingBufferSpec);

            stagingBuffer.MapMemory(indexBufferSpec.InstanceSize);
            stagingBuffer.WriteToBuffer(indices, indexBufferSpec.InstanceSize, 0);
            stagingBuffer.UnmapMemory();

            RenderCommand::CopyBuffer(stagingBuffer.GetBuffer(), s_Renderer2DData.QuadIndexBuffer->GetBuffer(), indexBufferSpec.InstanceSize);

            delete[] indices;

            VkDescriptorSetLayout descriptorSetLayouts[] = { globalDescriptorSetLayout, Texture2D::GetDescriptorLayout()->GetLayout() };

            VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
            vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            vk_pipeline_layout_create_info.setLayoutCount = 2;
            vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;
            vk_pipeline_layout_create_info.pushConstantRangeCount = 0;
            vk_pipeline_layout_create_info.pPushConstantRanges = nullptr;

            VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &s_Renderer2DData.QuadPipelineLayout));

            PipelineSpecification pipelineSpec{};
            pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/quad.vert.spv";
            pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/quad.frag.spv";
            pipelineSpec.RenderPass = renderPass;
            pipelineSpec.PipelineLayout = s_Renderer2DData.QuadPipelineLayout;

            pipelineSpec.VertexLayout = {
                VertexInputAttribute::VEC3F, // a_Position
                VertexInputAttribute::VEC3F, // a_Color
            };
            pipelineSpec.VertexInputBindingDescription.binding = 0;
            pipelineSpec.VertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            pipelineSpec.VertexInputBindingDescription.stride = sizeof(QuadVertex);
            pipelineSpec.Samples = RenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());
            pipelineSpec.BlendingEnable = true;

            pipelineSpec.CullMode = VK_CULL_MODE_FRONT_BIT;

            s_Renderer2DData.QuadPipeline = Pipeline::Create(pipelineSpec);
        }

        s_Renderer2DData.LightIconTexture = Texture2D::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/bulb_icon_v4.png");
    }

    void Renderer2D::AddGrid(int gridSize)
    {
        constexpr float squareSize = 1.0f;
        const float length = 2 * gridSize * squareSize;

        const float start = -gridSize * squareSize;

        float current = start;

        constexpr glm::vec3 lineColor(0.35f);
        constexpr glm::vec3 boldLineColor(0.7f);
        constexpr glm::vec3 redLineColor(1.0f, 0.235f, 0.286f);
        constexpr glm::vec3 blueLineColor(0.286f, 0.235f, 1.0f);

        for (int i = -gridSize; i <= gridSize; i++)
        {
            bool is_bold = i == 0;

            Renderer2D::AddLine({ current, 0.0f, start }, { current, 0.0f, start + length }, is_bold ? blueLineColor : lineColor);
            Renderer2D::AddLine({ start, 0.0f, current }, { start + length, 0.0f, current }, is_bold ? redLineColor : lineColor);
            current += squareSize;
        }
    }

    void Renderer2D::AddLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color)
    {
        s_Renderer2DData.LineVertices.push_back(LineVertex{ start, color });
        s_Renderer2DData.LineVertices.push_back(LineVertex{ end, color });
    }

    void Renderer2D::AddBillboard(const glm::vec3& position, float size, const glm::vec3& color, const glm::mat4& viewMatrix)
    {
        constexpr glm::vec2 m_GenericVertexOffsets[] = {
            {-1.0f, -1.0f},
            { 1.0f, -1.0f},
            { 1.0f,  1.0f},
            {-1.0f,  1.0f}
        };

        const glm::vec3& right = { viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0] };
        const glm::vec3& up = { viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1] };

        for (uint8_t i = 0; i < 4; i++)
        {
            s_Renderer2DData.QuadVertices.push_back(
                QuadVertex{
                    .Position = glm::vec3(position
                    + size * m_GenericVertexOffsets[i].x * right
                    - size * m_GenericVertexOffsets[i].y * up),
                    .Color = color
                }
            );
        }
    }

    void Renderer2D::Render(VkDescriptorSet globalDescriptorSet)
    {
        if (s_Renderer2DData.LineVertices.size())
        {
            FL_ASSERT(s_Renderer2DData.LineVertices.size() <= 2 * MAX_LINES, "MAX_LINES limit reached!");
            s_Renderer2DData.LineVertexBuffer->WriteToBuffer(s_Renderer2DData.LineVertices.data(), s_Renderer2DData.LineVertices.size() * sizeof(LineVertex), 0);
            s_Renderer2DData.LinePipeline->Bind();

            uint32_t vertexCount = s_Renderer2DData.LineVertices.size();
            auto vertexBuffer = s_Renderer2DData.LineVertexBuffer->GetBuffer();
            auto pipelineLayout = s_Renderer2DData.LinePipelineLayout;

            Renderer::Submit([pipelineLayout, globalDescriptorSet, vertexBuffer, vertexCount](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                {
                    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &globalDescriptorSet, 0, nullptr);

                    VkDeviceSize offsets[] = { 0 };
                    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);

                    vkCmdDraw(cmdBuffer, vertexCount, 1, 0, 0);
                }
            );
            s_Renderer2DData.LineVertices.clear();
        }

        if (s_Renderer2DData.QuadVertices.size())
        {
            FL_ASSERT(s_Renderer2DData.QuadVertices.size() <= MAX_QUAD_VERTICES, "MAX_QUAD_VERTICES limit reached!");
            s_Renderer2DData.QuadVertexBuffer->WriteToBuffer(s_Renderer2DData.QuadVertices.data(), s_Renderer2DData.QuadVertices.size() * sizeof(QuadVertex), 0);
            s_Renderer2DData.QuadPipeline->Bind();

            auto vertexBuffer = s_Renderer2DData.QuadVertexBuffer->GetBuffer();
            auto indexBuffer = s_Renderer2DData.QuadIndexBuffer->GetBuffer();
            auto pipelineLayout = s_Renderer2DData.QuadPipelineLayout;
            auto indexCount = 6 * (uint32_t)s_Renderer2DData.QuadVertices.size() / 4;

            Renderer::Submit([pipelineLayout, globalDescriptorSet, vertexBuffer, indexBuffer, indexCount](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                {
                    VkDescriptorSet descriptorSets[] = { globalDescriptorSet, s_Renderer2DData.LightIconTexture->GetDescriptorSet() };
                    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 2, descriptorSets, 0, nullptr);

                    VkDeviceSize offsets[] = { 0 };
                    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);
                    vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);
                }
            );
            s_Renderer2DData.QuadVertices.clear();
        }
    }

    void Renderer2D::Destroy()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyPipelineLayout(device, s_Renderer2DData.LinePipelineLayout, nullptr);
        s_Renderer2DData.LinePipeline = nullptr;
        s_Renderer2DData.LineVertexBuffer = nullptr;

        vkDestroyPipelineLayout(device, s_Renderer2DData.QuadPipelineLayout, nullptr);
        s_Renderer2DData.QuadPipeline = nullptr;
        s_Renderer2DData.QuadVertexBuffer = nullptr;
        s_Renderer2DData.QuadIndexBuffer = nullptr;

        // Temp
        s_Renderer2DData.LightIconTexture = nullptr;
    }
}
