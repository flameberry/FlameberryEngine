#include "SceneRenderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanDebug.h"
#include "VulkanRenderer.h"
#include "VulkanRenderCommand.h"
#include "StaticMesh.h"
#include "Renderer/Material.h"

#include "ECS/Component.h"
#include "AssetManager/AssetManager.h"

namespace Flameberry {
    struct SceneUniformBufferData {
        glm::vec3 cameraPosition;
        DirectionalLight directionalLight;
        PointLight pointLights[10];
        int lightCount = 0;
    };

    struct MeshData {
        alignas(16) glm::mat4 ModelMatrix;
        alignas(16) glm::vec3 Albedo{ 1.0f };
        alignas(4) float Roughness = 0.2f;
        alignas(4) float Metallic = 0.0f;
        alignas(4) float TextureMapEnabled = 0.0f;
        alignas(4) float NormalMapEnabled = 0.0f;
    };

    SceneRenderer::SceneRenderer(const std::shared_ptr<VulkanDescriptorPool>& globalDescriptorPool, VkDescriptorSetLayout globalDescriptorLayout, const std::shared_ptr<RenderPass>& renderPass)
        : m_GlobalDescriptorPool(globalDescriptorPool)
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        // Creating Uniform Buffers
        VkDeviceSize uniformBufferSize = sizeof(SceneUniformBufferData);
        for (auto& uniformBuffer : m_SceneUniformBuffers)
        {
            uniformBuffer = std::make_unique<Flameberry::VulkanBuffer>(uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            uniformBuffer->MapMemory(uniformBufferSize);
        }

        VkDescriptorSetLayoutBinding sceneDataBinding{};
        sceneDataBinding.binding = 0;
        sceneDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sceneDataBinding.descriptorCount = 1;
        sceneDataBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        sceneDataBinding.pImmutableSamplers = nullptr;

        std::vector<VkDescriptorSetLayoutBinding> bindings = { sceneDataBinding };

        m_SceneDescriptorLayout = std::make_unique<Flameberry::VulkanDescriptorLayout>(bindings);
        m_SceneDescriptorWriter = std::make_unique<Flameberry::VulkanDescriptorWriter>(*m_SceneDescriptorLayout);

        m_SceneDataDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo vk_descriptor_lighting_buffer_info{};
            vk_descriptor_lighting_buffer_info.buffer = m_SceneUniformBuffers[i]->GetBuffer();
            vk_descriptor_lighting_buffer_info.offset = 0;
            vk_descriptor_lighting_buffer_info.range = sizeof(SceneUniformBufferData);

            m_GlobalDescriptorPool->AllocateDescriptorSet(&m_SceneDataDescriptorSets[i], m_SceneDescriptorLayout->GetLayout());
            m_SceneDescriptorWriter->WriteBuffer(0, &vk_descriptor_lighting_buffer_info);
            m_SceneDescriptorWriter->Update(m_SceneDataDescriptorSets[i]);
        }

        VkPushConstantRange vk_push_constant_range{};
        vk_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        vk_push_constant_range.size = sizeof(MeshData);
        vk_push_constant_range.offset = 0;

        VkDescriptorSetLayout descriptorSetLayouts[] = { globalDescriptorLayout, m_SceneDescriptorLayout->GetLayout(), VulkanTexture::GetDescriptorLayout()->GetLayout(), VulkanTexture::GetDescriptorLayout()->GetLayout() };
        VkPushConstantRange pushConstantRanges[] = { vk_push_constant_range };

        VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
        vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vk_pipeline_layout_create_info.setLayoutCount = sizeof(descriptorSetLayouts) / sizeof(VkDescriptorSetLayout);
        vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;
        vk_pipeline_layout_create_info.pushConstantRangeCount = sizeof(pushConstantRanges) / sizeof(VkPushConstantRange);
        vk_pipeline_layout_create_info.pPushConstantRanges = pushConstantRanges;

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_VkPipelineLayout));

        Flameberry::PipelineSpecification pipelineSpec{};
        pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/triangleVert.spv";
        pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/triangleFrag.spv";
        pipelineSpec.RenderPass = renderPass;
        pipelineSpec.PipelineLayout = m_VkPipelineLayout;

        pipelineSpec.VertexLayout = {
            VertexInputAttribute::VEC3F, // a_Position
            VertexInputAttribute::VEC3F, // a_Normal
            VertexInputAttribute::VEC2F, // a_TextureCoords
            VertexInputAttribute::VEC3F, // a_Tangent
            VertexInputAttribute::VEC3F  // a_BiTangent
        };
        pipelineSpec.VertexInputBindingDescription = Flameberry::VulkanVertex::GetBindingDescription();
        pipelineSpec.Samples = VulkanRenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());

        m_MeshPipeline = std::make_unique<Flameberry::Pipeline>(pipelineSpec);
    }

    void SceneRenderer::OnDraw(VkCommandBuffer commandBuffer, uint32_t currentFrameIndex, VkDescriptorSet globalDescriptorSet, const PerspectiveCamera& activeCamera, const std::shared_ptr<Scene>& scene)
    {
        m_MeshPipeline->Bind(commandBuffer);

        // Updating Scene Uniform Buffer
        SceneUniformBufferData sceneUniformBufferData;
        sceneUniformBufferData.cameraPosition = activeCamera.GetPosition();
        sceneUniformBufferData.directionalLight = scene->m_SceneData.ActiveEnvironmentMap.DirLight;
        for (const auto& entity : scene->m_Registry->view<TransformComponent, LightComponent>())
        {
            const auto& [transform, light] = scene->m_Registry->get<TransformComponent, LightComponent>(entity);
            sceneUniformBufferData.pointLights[sceneUniformBufferData.lightCount].Position = transform.translation;
            sceneUniformBufferData.pointLights[sceneUniformBufferData.lightCount].Color = light.Color;
            sceneUniformBufferData.pointLights[sceneUniformBufferData.lightCount].Intensity = light.Intensity;
            sceneUniformBufferData.lightCount++;
        }
        // sceneUniformBufferData.EnvironmentMapReflections = scene->m_SceneData.ActiveEnvironmentMap.Reflections;

        m_SceneUniformBuffers[currentFrameIndex]->WriteToBuffer(&sceneUniformBufferData, sizeof(SceneUniformBufferData), 0);

        VkDescriptorSet descriptorSets[] = { globalDescriptorSet, m_SceneDataDescriptorSets[currentFrameIndex] };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkPipelineLayout, 0, sizeof(descriptorSets) / sizeof(VkDescriptorSet), descriptorSets, 0, nullptr);

        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

            if (!mesh.MeshUUID) // TODO: MeshUUID should never be invalid either it should point to default mesh or user loaded mesh
                continue;

            const auto& staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshUUID);
            staticMesh->Bind(commandBuffer);

            int submeshIndex = 0;
            for (const auto& submesh : staticMesh->GetSubMeshes()) {
                MeshData pushConstantMeshData;
                pushConstantMeshData.ModelMatrix = transform.GetTransform();

                bool isMaterialHandleValid = AssetManager::IsAssetHandleValid(submesh.MaterialUUID);
                std::shared_ptr<Material> materialAsset;
                if (isMaterialHandleValid) {
                    materialAsset = AssetManager::GetAsset<Material>(submesh.MaterialUUID);
                    pushConstantMeshData.Albedo = materialAsset->Albedo;
                    pushConstantMeshData.Roughness = materialAsset->Roughness;
                    pushConstantMeshData.Metallic = materialAsset->Metallic;
                    pushConstantMeshData.TextureMapEnabled = materialAsset->TextureMapEnabled;
                    pushConstantMeshData.NormalMapEnabled = materialAsset->NormalMapEnabled;
                }

                std::vector<VkDescriptorSet> materialDescriptorSets;
                materialDescriptorSets.resize(2, VulkanTexture::GetEmptyDescriptorSet());

                if (isMaterialHandleValid) {
                    if (materialAsset->TextureMapEnabled)
                        materialDescriptorSets[0] = materialAsset->TextureMap->GetDescriptorSet();
                    if (materialAsset->NormalMapEnabled)
                        materialDescriptorSets[1] = materialAsset->NormalMap->GetDescriptorSet();
                }

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkPipelineLayout, 2, materialDescriptorSets.size(), materialDescriptorSets.data(), 0, nullptr);
                vkCmdPushConstants(commandBuffer, m_VkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshData), &pushConstantMeshData);

                staticMesh->OnDrawSubMesh(commandBuffer, submeshIndex);
                submeshIndex++;
            }
        }
    }

    void SceneRenderer::OnDrawForShadowPass(VkCommandBuffer commandBuffer, VkPipelineLayout shadowMapPipelineLayout, const PerspectiveCamera& activeCamera, const std::shared_ptr<Scene>& scene)
    {
        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

            if (!mesh.MeshUUID)
                continue;

            ModelMatrixPushConstantData pushContantData;
            pushContantData.ModelMatrix = transform.GetTransform();
            vkCmdPushConstants(commandBuffer, shadowMapPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrixPushConstantData), &pushContantData);

            const auto& staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshUUID);
            staticMesh->Bind(commandBuffer);
            staticMesh->OnDraw(commandBuffer);
        }
    }

    void SceneRenderer::OnDrawForMousePickingPass(VkCommandBuffer commandBuffer, VkPipelineLayout mousePickingPipelineLayout, const PerspectiveCamera& activeCamera, const std::shared_ptr<Scene>& scene)
    {
        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

            if (!mesh.MeshUUID)
                continue;

            MousePickingPushConstantData pushContantData;
            pushContantData.ModelMatrix = transform.GetTransform();
            pushContantData.EntityID = (int)entity;
            vkCmdPushConstants(commandBuffer, mousePickingPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MousePickingPushConstantData), &pushContantData);

            const auto& staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshUUID);
            staticMesh->Bind(commandBuffer);
            staticMesh->OnDraw(commandBuffer);
        }
    }

    SceneRenderer::~SceneRenderer()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyPipelineLayout(device, m_VkPipelineLayout, nullptr);
    }
}
