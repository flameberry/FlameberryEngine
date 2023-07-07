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
    struct SceneUniformBufferData {
        alignas(64) glm::mat4 cascadeViewProjectionMatrix[SHADOW_MAP_CASCADE_COUNT];
        alignas(16) float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];
        alignas(16) glm::vec3 cameraPosition;
        alignas(16) DirectionalLight directionalLight;
        alignas(16) PointLight pointLights[10];
        alignas(4)  int lightCount = 0;
        alignas(4)  int showCascades = 0;
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

    SceneRenderer::SceneRenderer(const std::shared_ptr<DescriptorSetLayout>& globalDescriptorLayout, const std::shared_ptr<RenderPass>& renderPass, const std::vector<VkImageView>& shadowMapImageViews, VkSampler shadowMapSampler)
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        {
            // Creating Mesh Pipeline
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

            PipelineSpecification pipelineSpec{};
            pipelineSpec.PipelineLayout.PushConstants = {
                { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(MeshData) }
            };

            pipelineSpec.PipelineLayout.DescriptorSetLayouts = {
                globalDescriptorLayout,
                m_SceneDescriptorSetLayout,
                Texture2D::GetDescriptorLayout(),
                Texture2D::GetDescriptorLayout(),
                Texture2D::GetDescriptorLayout(),
                Texture2D::GetDescriptorLayout(),
                Texture2D::GetDescriptorLayout()
            };

            pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/triangle.vert.spv";
            pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/triangle.frag.spv";
            pipelineSpec.RenderPass = renderPass;

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

            pipelineSpec.PipelineLayout.DescriptorSetLayouts = { globalDescriptorLayout };

            pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/outline.vert.spv";
            pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/outline.frag.spv";
            pipelineSpec.RenderPass = renderPass;

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
            pipelineSpec.RenderPass = renderPass;

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
    }

    void SceneRenderer::OnDraw(
        VkDescriptorSet globalDescriptorSet,
        const PerspectiveCamera& activeCamera,
        const std::shared_ptr<Scene>& scene,
        const glm::vec2& framebufferSize,
        const std::array<glm::mat4, SHADOW_MAP_CASCADE_COUNT>& cascadeMatrices,
        const std::array<float, SHADOW_MAP_CASCADE_COUNT>& cascadeSplits,
        bool colorCascades,
        const fbentt::entity& selectedEntity
    )
    {
        uint32_t currentFrameIndex = Renderer::GetCurrentFrameIndex();

        // Skybox Rendering
        if (scene->m_SceneData.ActiveEnvironment.EnableEnvironmentMap && scene->m_SceneData.ActiveEnvironment.EnvironmentMap)
        {
            m_SkyboxPipeline->Bind();
            glm::mat4 viewProjectionMatrix = activeCamera.GetProjectionMatrix() * glm::mat4(glm::mat3(activeCamera.GetViewMatrix()));

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

        m_MeshPipeline->Bind();

        // Updating Scene Uniform Buffer
        SceneUniformBufferData sceneUniformBufferData;
        sceneUniformBufferData.cameraPosition = activeCamera.GetSpecification().Position;
        sceneUniformBufferData.directionalLight = scene->m_SceneData.ActiveEnvironment.DirLight;
        for (const auto& entity : scene->m_Registry->view<TransformComponent, LightComponent>())
        {
            const auto& [transform, light] = scene->m_Registry->get<TransformComponent, LightComponent>(entity);
            sceneUniformBufferData.pointLights[sceneUniformBufferData.lightCount].Position = transform.translation;
            sceneUniformBufferData.pointLights[sceneUniformBufferData.lightCount].Color = light.Color;
            sceneUniformBufferData.pointLights[sceneUniformBufferData.lightCount].Intensity = light.Intensity;
            sceneUniformBufferData.lightCount++;

            Renderer2D::AddBillboard(transform.translation, 0.7f, glm::vec3(1.0f), activeCamera.GetViewMatrix());
        }
        for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            sceneUniformBufferData.cascadeViewProjectionMatrix[i] = cascadeMatrices[i];
            sceneUniformBufferData.cascadeSplits[i] = cascadeSplits[i];
        }
        sceneUniformBufferData.showCascades = (int)colorCascades;
        // sceneUniformBufferData.EnvironmentMapReflections = scene->m_SceneData.ActiveEnvironment.Reflections;

        m_SceneUniformBuffers[currentFrameIndex]->WriteToBuffer(&sceneUniformBufferData, sizeof(SceneUniformBufferData), 0);

        // Draw Scene Meshes
        VkDescriptorSet descriptorSets[] = { globalDescriptorSet, m_SceneDataDescriptorSets[currentFrameIndex]->GetDescriptorSet() };
        const auto& pipelineLayout = m_MeshPipeline->GetLayout();

        Renderer::Submit([pipelineLayout, descriptorSets](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, sizeof(descriptorSets) / sizeof(VkDescriptorSet), descriptorSets, 0, nullptr);
            }
        );

        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            if (entity == selectedEntity)
                continue;

            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

            SubmitMesh(mesh.MeshHandle, mesh.OverridenMaterialTable, transform.GetTransform());
        }

        // Draw Outlined Object
        if (selectedEntity != fbentt::null && scene->m_Registry->has<MeshComponent>(selectedEntity))
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(selectedEntity);

            auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle);
            if (staticMesh) {
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

                SubmitMesh(mesh.MeshHandle, mesh.OverridenMaterialTable, transform.GetTransform());

                m_OutlinePipeline->Bind();

                const auto& outlinePipelineLayout = m_OutlinePipeline->GetLayout();

                OutlinePushConstantData pushConstantData;
                pushConstantData.ModelMatrix = glm::scale(transform.GetTransform(), glm::vec3(1.05f));
                // pushConstantData.ModelMatrix = transform.GetTransform();
                pushConstantData.ScreenSize = framebufferSize;

                Renderer::Submit([outlinePipelineLayout, globalDescriptorSet, pushConstantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                    {
                        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, outlinePipelineLayout, 0, 1, &globalDescriptorSet, 0, nullptr);
                        vkCmdPushConstants(cmdBuffer, outlinePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(OutlinePushConstantData), &pushConstantData);
                    }
                );

                staticMesh->OnDraw();
            }
        }
    }

    void SceneRenderer::OnDrawForShadowPass(VkPipelineLayout shadowMapPipelineLayout, const std::shared_ptr<Scene>& scene)
    {
        for (const auto& entity : scene->m_Registry->view<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = scene->m_Registry->get<TransformComponent, MeshComponent>(entity);

            auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle);
            if (staticMesh)
            {
                ModelMatrixPushConstantData pushContantData;
                pushContantData.ModelMatrix = transform.GetTransform();
                Renderer::Submit([shadowMapPipelineLayout, pushContantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                    {
                        vkCmdPushConstants(cmdBuffer, shadowMapPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrixPushConstantData), &pushContantData);
                    }
                );

                staticMesh->Bind();
                staticMesh->OnDraw();
            }
        }
    }

    void SceneRenderer::OnDrawForMousePickingPass(VkPipelineLayout mousePickingPipelineLayout, const std::shared_ptr<Scene>& scene)
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

                Renderer::Submit([mousePickingPipelineLayout, pushContantData](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
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

        staticMesh->Bind();

        std::vector<VkDescriptorSet> materialDescriptorSets(5);

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

            const auto& pipelineLayout = m_MeshPipeline->GetLayout();
            Renderer::Submit([pipelineLayout, materialDescriptorSets, pushConstantMeshData](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
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
