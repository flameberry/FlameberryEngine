#include "VulkanRenderCommand.h"

#include <fstream>

#include "Core/Core.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

namespace Flameberry {
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
        FL_ASSERT(vkCreateShaderModule(device, &vk_shader_module_create_info, nullptr, &shaderModule) == VK_SUCCESS, "Failed to create shader module!");
        return shaderModule;
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
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        std::vector<VulkanVertex> vertices;
        std::vector<uint32_t> indices;

        FL_ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str()), err);

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

                vertex.textureUV = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = { 1.0f, 1.0f, 1.0f, 1.0f };

                vertices.push_back(vertex);
                indices.push_back(indices.size());
            }
        }
        return std::tuple<std::vector<VulkanVertex>, std::vector<uint32_t>>(vertices, indices);
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
