#include "SceneRenderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Vulkan/VulkanDebug.h"
#include "Vulkan/VulkanRenderCommand.h"
#include "Vulkan/VulkanContext.h"
#include "Material.h"
#include "Renderer.h"

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

    SceneRenderer::SceneRenderer(VkDescriptorSetLayout globalDescriptorLayout, const std::shared_ptr<RenderPass>& renderPass, const std::vector<VkImageView>& shadowMapImageViews, VkSampler shadowMapSampler)
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        // Creating Mesh Pipeline
        {
            VkDeviceSize uniformBufferSize = sizeof(SceneUniformBufferData);

            BufferSpecification bufferSpec;
            bufferSpec.InstanceCount = 1;
            bufferSpec.InstanceSize = uniformBufferSize;
            bufferSpec.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

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
            sceneDescSetLayoutSpec.Bindings[0].pImmutableSamplers = nullptr;

            sceneDescSetLayoutSpec.Bindings[1].binding = 1;
            sceneDescSetLayoutSpec.Bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            sceneDescSetLayoutSpec.Bindings[1].descriptorCount = 1;
            sceneDescSetLayoutSpec.Bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            sceneDescSetLayoutSpec.Bindings[1].pImmutableSamplers = nullptr;

            m_SceneDescriptorSetLayout = DescriptorSetLayout::Create(sceneDescSetLayoutSpec);

            DescriptorSetSpecification sceneDescSetSpec;
            sceneDescSetSpec.Layout = m_SceneDescriptorSetLayout;

            m_SceneDataDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
            for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
            {
                m_SceneDataDescriptorSets[i] = DescriptorSet::Create(sceneDescSetSpec);

                VkDescriptorBufferInfo vk_descriptor_lighting_buffer_info{};
                vk_descriptor_lighting_buffer_info.buffer = m_SceneUniformBuffers[i]->GetBuffer();
                vk_descriptor_lighting_buffer_info.offset = 0;
                vk_descriptor_lighting_buffer_info.range = sizeof(SceneUniformBufferData);

                VkDescriptorImageInfo vk_shadow_map_image_info{};
                vk_shadow_map_image_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                vk_shadow_map_image_info.imageView = shadowMapImageViews[i];
                vk_shadow_map_image_info.sampler = shadowMapSampler;

                m_SceneDataDescriptorSets[i]->WriteBuffer(0, vk_descriptor_lighting_buffer_info);
                m_SceneDataDescriptorSets[i]->WriteImage(1, vk_shadow_map_image_info);
                m_SceneDataDescriptorSets[i]->Update();
            }

            VkPushConstantRange vk_push_constant_range{};
            vk_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            vk_push_constant_range.size = sizeof(MeshData);
            vk_push_constant_range.offset = 0;

            VkDescriptorSetLayout descriptorSetLayouts[] = { globalDescriptorLayout, m_SceneDescriptorSetLayout->GetLayout(), VulkanTexture::GetDescriptorLayout()->GetLayout(), VulkanTexture::GetDescriptorLayout()->GetLayout() };
            VkPushConstantRange pushConstantRanges[] = { vk_push_constant_range };

            VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
            vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            vk_pipeline_layout_create_info.setLayoutCount = sizeof(descriptorSetLayouts) / sizeof(VkDescriptorSetLayout);
            vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;
            vk_pipeline_layout_create_info.pushConstantRangeCount = sizeof(pushConstantRanges) / sizeof(VkPushConstantRange);
            vk_pipeline_layout_create_info.pPushConstantRanges = pushConstantRanges;

            VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_VkPipelineLayout));

            PipelineSpecification pipelineSpec{};
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
            pipelineSpec.VertexInputBindingDescription = VulkanVertex::GetBindingDescription();
            pipelineSpec.Samples = VulkanRenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());

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
            VkPushConstantRange vk_push_constant_range{};
            vk_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            vk_push_constant_range.size = sizeof(ModelMatrixPushConstantData);
            vk_push_constant_range.offset = 0;

            VkDescriptorSetLayout descriptorSetLayouts[] = { globalDescriptorLayout };
            VkPushConstantRange pushConstantRanges[] = { vk_push_constant_range };

            VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
            vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            vk_pipeline_layout_create_info.setLayoutCount = sizeof(descriptorSetLayouts) / sizeof(VkDescriptorSetLayout);
            vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;
            vk_pipeline_layout_create_info.pushConstantRangeCount = sizeof(pushConstantRanges) / sizeof(VkPushConstantRange);
            vk_pipeline_layout_create_info.pPushConstantRanges = pushConstantRanges;

            VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_OutlinePipelineLayout));

            PipelineSpecification pipelineSpec{};
            pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/outlineVert.spv";
            pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/outlineFrag.spv";
            pipelineSpec.RenderPass = renderPass;
            pipelineSpec.PipelineLayout = m_OutlinePipelineLayout;

            pipelineSpec.VertexLayout = {
                VertexInputAttribute::VEC3F, // a_Position
            };
            pipelineSpec.VertexInputBindingDescription = VulkanVertex::GetBindingDescription();
            pipelineSpec.Samples = VulkanRenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());

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
    }

    void SceneRenderer::OnDraw(VkDescriptorSet globalDescriptorSet, const PerspectiveCamera& activeCamera, const std::shared_ptr<Scene>& scene, const glm::vec2& framebufferSize, const fbentt::entity& selectedEntity)
    {
        uint32_t currentFrameIndex = Renderer::GetCurrentFrameIndex();
        m_MeshPipeline->Bind();

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

        VkDescriptorSet descriptorSets[] = { globalDescriptorSet, m_SceneDataDescriptorSets[currentFrameIndex]->GetDescriptorSet() };
        const auto& pipelineLayout = m_VkPipelineLayout;

        Renderer::Submit([pipelineLayout, descriptorSets](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, sizeof(descriptorSets) / sizeof(VkDescriptorSet), descriptorSets, 0, nullptr);
            }
        );

        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            if (entity == selectedEntity)
                continue;

            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

            if (!mesh.MeshUUID) // TODO: MeshUUID should never be invalid either it should point to default mesh or user loaded mesh
                continue;

            const auto& staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshUUID);
            staticMesh->Bind();

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

                Renderer::Submit([pipelineLayout, materialDescriptorSets, pushConstantMeshData](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
                    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, materialDescriptorSets.size(), materialDescriptorSets.data(), 0, nullptr);
                    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshData), &pushConstantMeshData);
                    }
                );

                staticMesh->OnDrawSubMesh(submeshIndex);
                submeshIndex++;
            }
        }

        // Draw Outlined Object
        if (selectedEntity != fbentt::null && scene->m_Registry->has<MeshComponent>(selectedEntity))
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(selectedEntity);

            if (!mesh.MeshUUID) // TODO: MeshUUID should never be invalid either it should point to default mesh or user loaded mesh
                return;

            auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshUUID);

            Renderer::Submit([framebufferSize](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
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

            staticMesh->Bind();

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

                Renderer::Submit([pipelineLayout, materialDescriptorSets, pushConstantMeshData](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
                    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, materialDescriptorSets.size(), materialDescriptorSets.data(), 0, nullptr);
                    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshData), &pushConstantMeshData);
                    }
                );

                staticMesh->OnDrawSubMesh(submeshIndex);
                submeshIndex++;
            }

            m_OutlinePipeline->Bind();

            const auto& outlinePipelineLayout = m_OutlinePipelineLayout;

            const float borderWidth = 3.0f;

            ModelMatrixPushConstantData pushConstantData;
            pushConstantData.ModelMatrix = glm::scale(transform.GetTransform(), glm::vec3(1.1f, 1.1f, 1.1f));
            Renderer::Submit([outlinePipelineLayout, globalDescriptorSet, pushConstantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
                vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, outlinePipelineLayout, 0, 1, &globalDescriptorSet, 0, nullptr);
                vkCmdPushConstants(cmdBuffer, outlinePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrixPushConstantData), &pushConstantData);
                }
            );

            staticMesh->OnDraw();
        }
    }

    void SceneRenderer::OnDrawForShadowPass(VkPipelineLayout shadowMapPipelineLayout, const std::shared_ptr<Scene>& scene)
    {
        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

            if (!mesh.MeshUUID)
                continue;

            ModelMatrixPushConstantData pushContantData;
            pushContantData.ModelMatrix = transform.GetTransform();
            Renderer::Submit([shadowMapPipelineLayout, pushContantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
                vkCmdPushConstants(cmdBuffer, shadowMapPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrixPushConstantData), &pushContantData);
                }
            );

            const auto& staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshUUID);
            staticMesh->Bind();
            staticMesh->OnDraw();
        }
    }

    void SceneRenderer::OnDrawForMousePickingPass(VkPipelineLayout mousePickingPipelineLayout, const std::shared_ptr<Scene>& scene)
    {
        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

            if (!mesh.MeshUUID)
                continue;

            MousePickingPushConstantData pushContantData;
            pushContantData.ModelMatrix = transform.GetTransform();
            pushContantData.EntityIndex = fbentt::to_index(entity);

            Renderer::Submit([mousePickingPipelineLayout, pushContantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
                vkCmdPushConstants(cmdBuffer, mousePickingPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MousePickingPushConstantData), &pushContantData);
                }
            );

            const auto& staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshUUID);
            staticMesh->Bind();
            staticMesh->OnDraw();
        }
    }

    SceneRenderer::~SceneRenderer()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyPipelineLayout(device, m_VkPipelineLayout, nullptr);
        vkDestroyPipelineLayout(device, m_OutlinePipelineLayout, nullptr);
    }
}
