#include "VulkanRenderCommand.h"

#include <fstream>
#include <unordered_map>

#include "VulkanDebug.h"
#include "Core/Timer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

namespace Flameberry {
    template <typename T, typename... Rest>
    void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
        seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);
    };
}

namespace std {
    template <>
    struct hash<Flameberry::VulkanVertex> {
        size_t operator()(Flameberry::VulkanVertex const& vertex) const {
            size_t seed = 0;
            Flameberry::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.textureUV);
            return seed;
        }
    };
}

namespace Flameberry {
    void VulkanRenderCommand::SetViewport(VkCommandBuffer commandBuffer, float x, float y, float width, float height)
    {
        VkViewport viewport{};
        viewport.x = x;
        viewport.y = y;
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    }

    void VulkanRenderCommand::SetScissor(VkCommandBuffer commandBuffer, VkOffset2D offset, VkExtent2D extent)
    {
        VkRect2D scissor{};
        scissor.offset = offset;
        scissor.extent = extent;

        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void VulkanRenderCommand::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
    {
        const auto& device = VulkanContext::GetCurrentDevice();

        VkCommandBuffer commandBuffer;
        device->BeginSingleTimeCommandBuffer(commandBuffer);
        VkBufferCopy vk_buffer_copy_info{};
        vk_buffer_copy_info.srcOffset = 0;
        vk_buffer_copy_info.dstOffset = 0;
        vk_buffer_copy_info.size = bufferSize;

        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &vk_buffer_copy_info);
        device->EndSingleTimeCommandBuffer(commandBuffer);
    }

    VkShaderModule VulkanRenderCommand::CreateShaderModule(VkDevice device, const std::vector<char>& compiledShaderCode)
    {
        VkShaderModuleCreateInfo vk_shader_module_create_info{};
        vk_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vk_shader_module_create_info.codeSize = compiledShaderCode.size();
        vk_shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(compiledShaderCode.data());

        VkShaderModule shaderModule;
        VK_CHECK_RESULT(vkCreateShaderModule(device, &vk_shader_module_create_info, nullptr, &shaderModule));
        return shaderModule;
    }

    VkSampleCountFlagBits VulkanRenderCommand::GetMaxUsableSampleCount(VkPhysicalDevice physicalDevice)
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
        return VK_SAMPLE_COUNT_1_BIT;
    }

    uint32_t VulkanRenderCommand::GetValidMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags vk_memory_property_flags)
    {
        VkPhysicalDeviceMemoryProperties vk_physical_device_mem_properties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &vk_physical_device_mem_properties);

        for (uint32_t i = 0; i < vk_physical_device_mem_properties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (vk_physical_device_mem_properties.memoryTypes[i].propertyFlags & vk_memory_property_flags) == vk_memory_property_flags)
                return i;
        }
        FL_ERROR("Failed to find valid memory type!");
        return -1;
    }

    bool VulkanRenderCommand::HasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkFormat VulkanRenderCommand::GetSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidateFormats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
    {
        for (const auto& format : candidateFormats)
        {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

            if ((tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
                ||
                (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags))
                return format;
        }
        FL_ERROR("Couldn't find supported format!");
        return VK_FORMAT_UNDEFINED;
    }

    bool VulkanRenderCommand::CheckValidationLayerSupport(const std::vector<const char*>& validationLayers)
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> available_layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, available_layers.data());

        for (const char* layerName : validationLayers)
        {
            bool layerFound = false;
            for (const auto& layerProperties : available_layers)
            {
                if (!strcmp(layerName, layerProperties.layerName))
                {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound)
            {
                FL_ERROR("Failed to find the layer named '{0}'!", layerName);
                return false;
            }
        }

        // Prints the names of the validation layers available
        std::string layer_list = "";
        for (uint16_t i = 0; i < layerCount; i++)
        {
            layer_list += available_layers[i].layerName;
            if (!(i == layerCount - 1))
                layer_list += ", ";
        }
        FL_INFO("Found the following Vulkan Validation Layers: {0}", layer_list);
        return true;
    }

    QueueFamilyIndices VulkanRenderCommand::GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> vk_queue_family_props(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, vk_queue_family_props.data());

        uint32_t index = 0;
        QueueFamilyIndices indices;
        for (const auto& vk_queue : vk_queue_family_props)
        {
            if (vk_queue.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.GraphicsSupportedQueueFamilyIndex = index;

            VkBool32 is_presentation_supported = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &is_presentation_supported);

            if (is_presentation_supported)
                indices.PresentationSupportedQueueFamilyIndex = index;

            if (indices.GraphicsSupportedQueueFamilyIndex.has_value() && indices.PresentationSupportedQueueFamilyIndex.has_value())
                break;
            index++;
        }
        return indices;
    }

    SwapChainDetails VulkanRenderCommand::GetSwapChainDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        SwapChainDetails vk_swap_chain_details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &vk_swap_chain_details.SurfaceCapabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

        if (formatCount)
        {
            vk_swap_chain_details.SurfaceFormats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, vk_swap_chain_details.SurfaceFormats.data());
        }

        uint32_t presentationModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentationModeCount, nullptr);

        if (presentationModeCount)
        {
            vk_swap_chain_details.PresentationModes.resize(presentationModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentationModeCount, vk_swap_chain_details.PresentationModes.data());
        }

        return vk_swap_chain_details;
    }

    std::tuple<std::vector<VulkanVertex>, std::vector<uint32_t>> VulkanRenderCommand::LoadModel(const std::string& filePath)
    {
        FL_SCOPED_TIMER("Load_Model");
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        std::vector<VulkanVertex> vertices;
        std::vector<uint32_t> indices;

        size_t pos = filePath.find_last_of('/');
        std::string mtlBaseDir = filePath.substr(0, pos);
        FL_ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str(), mtlBaseDir.c_str()), err);

        bool has_tex_coord = true;
        if (!attrib.texcoords.size())
            has_tex_coord = false;

        std::unordered_map<VulkanVertex, uint32_t> uniqueVertices{};
        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                VulkanVertex vertex{};

                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };

                if (has_tex_coord) {
                    vertex.textureUV = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                }

                vertex.color = { 1.0f, 1.0f, 1.0f, 1.0f };

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]); // TODO: Ineffecient Method
                // vertices.push_back(vertex);
                // indices.push_back(indices.size());
            }
        }
        FL_INFO("Loaded Model: '{0}': Vertices: {1}, Indices: {2}", filePath, vertices.size(), indices.size());
        return std::tuple<std::vector<VulkanVertex>, std::vector<uint32_t>>(vertices, indices);
    }

    VkSampler VulkanRenderCommand::CreateDefaultSampler()
    {
        VkSampler sampler;
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &sampler));
        return sampler;
    }

    std::vector<char> VulkanRenderCommand::LoadCompiledShaderCode(const std::string& filePath)
    {
        std::ifstream stream(filePath, std::ios::ate | std::ios::binary);
        FL_ASSERT(stream.is_open(), "Failed to open the file '{0}'", filePath);

        size_t fileSize = (size_t)stream.tellg();

        FL_INFO("File size of buffer taken from '{0}' is {1}", filePath, fileSize);

        std::vector<char> buffer(fileSize);

        stream.seekg(0);
        stream.read(buffer.data(), fileSize);

        stream.close();
        return buffer;
    }
}
