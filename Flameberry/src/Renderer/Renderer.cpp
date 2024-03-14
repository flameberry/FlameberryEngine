#include "Renderer.h"

#include <glm/gtc/type_ptr.hpp>

#include "Core/Application.h"
#include "Core/Profiler.h"

#include "VulkanContext.h"
#include "Asset/AssetManager.h"
#include "SwapChain.h"
#include "ShaderLibrary.h"
#include "Texture2D.h"
#include "MaterialAsset.h"

namespace Flameberry {

    std::vector<Renderer::Command> Renderer::s_CommandQueue;
    uint32_t Renderer::s_RT_FrameIndex = 0, Renderer::s_FrameIndex = 0;

    void Renderer::Init()
    {
        // Create the generic texture descriptor layout
        Texture2D::InitStaticResources();
        ShaderLibrary::Init();

        s_CommandQueue.reserve(5 * 1028 * 1028 / sizeof(Renderer::Command)); // 5 MB
    }

    void Renderer::Shutdown()
    {
        ShaderLibrary::Shutdown();
        Texture2D::DestroyStaticResources();

        DescriptorSetLayout::ClearCache(); // TODO: Maybe move this to somewhere obvious like VulkanDevice or Renderer
    }

    void Renderer::Submit(const Command&& cmd)
    {
        s_CommandQueue.push_back(cmd);
    }

    void Renderer::WaitAndRender()
    {
        // Update the Frame Index of the Main Thread
        s_FrameIndex = (s_FrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
        RT_Render();
    }

    void Renderer::RT_Render()
    {
        FBY_PROFILE_SCOPE("RT_RenderLoop");
        auto& window = Application::Get().GetWindow();
        const auto& device = VulkanContext::GetCurrentDevice();

        // Execute all Render Commands
        if (window.BeginFrame())
        {
            device->ResetCommandBuffer(s_RT_FrameIndex);
            device->BeginCommandBuffer(s_RT_FrameIndex);

            VkCommandBuffer commandBuffer = VulkanContext::GetCurrentDevice()->GetCommandBuffer(s_RT_FrameIndex);
            uint32_t imageIndex = window.GetImageIndex();

            int i = 0;
            for (auto& cmd : s_CommandQueue)
            {
                cmd(commandBuffer, imageIndex);
                i++;
            }
            device->EndCommandBuffer(s_RT_FrameIndex);
            window.SwapBuffers();
        }

        s_CommandQueue.clear();

        // Update the Frame Index of the Render Thread
        s_RT_FrameIndex = (s_RT_FrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::SubmitMeshWithMaterial(const Ref<StaticMesh>& mesh, const Ref<Pipeline>& pipeline, const MaterialTable& materialTable, const glm::mat4& transform)
    {
        Renderer::Submit([mesh, pipelineLayout = pipeline->GetVulkanPipelineLayout(), transform](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                Renderer::RT_BindMesh(cmdBuffer, mesh);
                vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(transform), glm::value_ptr(transform));
            }
        );

        int submeshIndex = 0;
        for (const auto& submesh : mesh->GetSubMeshes())
        {
            // The Assumption here is every mesh loaded will have a Material, i.e. materialAsset won't be nullptr
            Ref<MaterialAsset> materialAsset;
            if (auto it = materialTable.find(submeshIndex); it != materialTable.end())
                materialAsset = AssetManager::GetAsset<MaterialAsset>(it->second);
            else if (AssetManager::IsAssetHandleValid(submesh.MaterialHandle))
                materialAsset = AssetManager::GetAsset<MaterialAsset>(submesh.MaterialHandle);

            Renderer::Submit([pipelineLayout = pipeline->GetVulkanPipelineLayout(), materialAsset, submesh = mesh->GetSubMeshes()[submeshIndex]](VkCommandBuffer cmdBuffer, uint32_t)
                {
                    VkDescriptorSet descSetArray[materialAsset->GetUnderlyingMaterial()->m_DescriptorSets.size()];
                    int idx = 0;
                    for (const auto& set : materialAsset->GetUnderlyingMaterial()->m_DescriptorSets)
                    {
                        descSetArray[idx] = set->GetVulkanDescriptorSet();
                        idx++;
                    }
                    auto setNumber = materialAsset->GetUnderlyingMaterial()->m_StartSetIndex;
                    uint32_t count = idx;

                    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, materialAsset->GetUnderlyingMaterial()->GetPushConstantOffset(), materialAsset->GetUnderlyingMaterial()->GetUniformDataSize(), materialAsset->GetUnderlyingMaterial()->GetUniformDataPtr());
                    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, setNumber, count, &descSetArray[0], 0, nullptr);
                    vkCmdDrawIndexed(cmdBuffer, submesh.IndexCount, 1, submesh.IndexOffset, 0, 0);
                }
            );
            submeshIndex++;
        }
    }

    void Renderer::RT_BindMesh(VkCommandBuffer cmdBuffer, const Ref<StaticMesh>& mesh)
    {
        auto vertexBuffer = mesh->GetVertexBuffer()->GetVulkanBuffer(), indexBuffer = mesh->GetIndexBuffer()->GetVulkanBuffer();
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void Renderer::RT_BindPipeline(VkCommandBuffer cmdBuffer, VkPipeline pipeline)
    {
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

}
