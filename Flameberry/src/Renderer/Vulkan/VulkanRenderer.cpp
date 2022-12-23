#include "VulkanRenderer.h"

#include <cstdint>
#include <algorithm>
#include <chrono>
#include <thread>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Core/Core.h"
#include "Core/Timer.h"
#include "VulkanRenderCommand.h"

namespace Flameberry {
    VkInstance                   VulkanRenderer::s_VkInstance;
    VkDebugUtilsMessengerEXT     VulkanRenderer::s_VkDebugMessenger;
    VkPhysicalDevice             VulkanRenderer::s_VkPhysicalDevice = VK_NULL_HANDLE;
    VkDevice                     VulkanRenderer::s_VkDevice;
    VulkanRenderer::QueueFamilyIndices VulkanRenderer::s_QueueFamilyIndices;
    VkQueue                      VulkanRenderer::s_VkGraphicsQueue;
    VkQueue                      VulkanRenderer::s_VkPresentationQueue;
    VkSurfaceKHR                 VulkanRenderer::s_VkSurface;
    VkCommandPool                VulkanRenderer::s_VkCommandPool;
    std::vector<VkCommandBuffer> VulkanRenderer::s_VkCommandBuffers;
    std::vector<const char*>     VulkanRenderer::s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
    bool                         VulkanRenderer::s_EnableValidationLayers = true;
    size_t                       VulkanRenderer::s_CurrentFrame = 0;
    uint32_t                     VulkanRenderer::s_ImageIndex;

    std::unique_ptr<VulkanSwapChain> VulkanRenderer::s_SwapChain;

    GLFWwindow* VulkanRenderer::s_UserGLFWwindow;

#ifdef __APPLE__
    std::vector<const char*>     VulkanRenderer::s_VkDeviceExtensions = { "VK_KHR_portability_subset", VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#else
    std::vector<const char*>     VulkanRenderer::s_VkDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#endif

    static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* userData
    )
    {
        switch (messageSeverity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            FL_LOG("{0}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            FL_INFO("{0}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            FL_WARN("{0}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            FL_ERROR("{0}", pCallbackData->pMessage);
            break;
        default:
            FL_TRACE("{0}", pCallbackData->pMessage);
            break;
        }
        return VK_FALSE;
    }

    void VulkanRenderer::Init(GLFWwindow* window)
    {
        FL_ASSERT(window, "GLFW Window provided to VulkanRenderer is null!");
        s_UserGLFWwindow = window;

        if (s_EnableValidationLayers)
            FL_ASSERT(CheckValidationLayerSupport(), "Validation Layers are requested but not available!");

        // Creating Vulkan Instance
        VkApplicationInfo vk_app_info{};
        vk_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        vk_app_info.pApplicationName = "Flameberry";
        vk_app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        vk_app_info.pEngineName = "No Engine";
        vk_app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        vk_app_info.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo vk_create_info{};
        vk_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        vk_create_info.pApplicationInfo = &vk_app_info;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> vk_extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        vk_extensions.push_back("VK_KHR_portability_enumeration");
        if (s_EnableValidationLayers)
            vk_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        vk_create_info.enabledExtensionCount = static_cast<uint32_t>(vk_extensions.size());
        vk_create_info.ppEnabledExtensionNames = vk_extensions.data();
        vk_create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        VkDebugUtilsMessengerCreateInfoEXT vk_debug_create_info{};

        if (s_EnableValidationLayers)
        {
            vk_create_info.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
            vk_create_info.ppEnabledLayerNames = s_ValidationLayers.data();

            vk_debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            vk_debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            vk_debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            vk_debug_create_info.pfnUserCallback = vk_debug_callback;

            vk_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&vk_debug_create_info;
        }
        else
        {
            vk_create_info.enabledLayerCount = 0;
            vk_create_info.pNext = nullptr;
        }

        FL_ASSERT(vkCreateInstance(&vk_create_info, nullptr, &s_VkInstance) == VK_SUCCESS, "Failed to create Vulkan Instance!");
        FL_INFO("Created Vulkan Instance!");

        if (s_EnableValidationLayers)
        {
            // Creating Vulkan Debug Messenger
            VkDebugUtilsMessengerCreateInfoEXT vk_debug_messenger_create_info{};
            vk_debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            vk_debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            vk_debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            vk_debug_messenger_create_info.pfnUserCallback = vk_debug_callback;
            vk_debug_messenger_create_info.pUserData = nullptr;

            FL_ASSERT(CreateDebugUtilsMessengerEXT(s_VkInstance, &vk_debug_messenger_create_info, nullptr, &s_VkDebugMessenger) == VK_SUCCESS, "Failed to created Vulkan Debug Messenger!");
            FL_INFO("Created Vulkan Debug Messenger!");
        }

        // Creating Vulkan Window Surface
        FL_ASSERT(glfwCreateWindowSurface(s_VkInstance, s_UserGLFWwindow, nullptr, &s_VkSurface) == VK_SUCCESS, "Failed to create window surface!");

        // Setting up Valid Vulkan Physical Device
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(s_VkInstance, &deviceCount, nullptr);

        FL_ASSERT(deviceCount, "Failed to find GPUs which support Vulkan!");

        std::vector<VkPhysicalDevice> vk_physical_devices(deviceCount);
        vkEnumeratePhysicalDevices(s_VkInstance, &deviceCount, vk_physical_devices.data());

        // Printing Physical Devices Names
        std::string physical_device_list = "";
        for (uint16_t i = 0; i < deviceCount; i++)
        {
            VkPhysicalDeviceProperties vk_physical_device_props;
            vkGetPhysicalDeviceProperties(vk_physical_devices[i], &vk_physical_device_props);
            physical_device_list += vk_physical_device_props.deviceName;
            if (i < deviceCount - 1)
                physical_device_list += ", ";
        }

        FL_INFO("{0} Physical devices found: {1}", deviceCount, physical_device_list);

        // Accessing the actual physical device
        s_VkPhysicalDevice = GetValidVkPhysicalDevice(vk_physical_devices);
        FL_ASSERT(s_VkPhysicalDevice != VK_NULL_HANDLE, "Vulkan physical device is null!");

        VkPhysicalDeviceProperties vk_physical_device_props;
        vkGetPhysicalDeviceProperties(s_VkPhysicalDevice, &vk_physical_device_props);
        FL_INFO("Selected Vulkan Physical Device: {0}", vk_physical_device_props.deviceName);

        // Getting Queue Family Indices
        s_QueueFamilyIndices = GetQueueFamilyIndices(s_VkPhysicalDevice);

        std::vector<VkDeviceQueueCreateInfo> vk_device_queue_create_infos = CreateDeviceQueueInfos(
            {
                s_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex,
                s_QueueFamilyIndices.PresentationSupportedQueueFamilyIndex
            }
        );

        VkPhysicalDeviceFeatures vk_physical_device_features{};
        vk_physical_device_features.samplerAnisotropy = VK_TRUE;

        // Creating Vulkan Logical Device
        VkDeviceCreateInfo vk_device_create_info{};
        vk_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        vk_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(vk_device_queue_create_infos.size());
        vk_device_create_info.pQueueCreateInfos = vk_device_queue_create_infos.data();

        vk_device_create_info.pEnabledFeatures = &vk_physical_device_features;

        vk_device_create_info.enabledExtensionCount = static_cast<uint32_t>(s_VkDeviceExtensions.size());
        vk_device_create_info.ppEnabledExtensionNames = s_VkDeviceExtensions.data();

        if (s_EnableValidationLayers)
        {
            vk_device_create_info.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
            vk_device_create_info.ppEnabledLayerNames = s_ValidationLayers.data();
        }
        else
        {
            vk_device_create_info.enabledLayerCount = 0;
        }

        FL_ASSERT(vkCreateDevice(s_VkPhysicalDevice, &vk_device_create_info, nullptr, &s_VkDevice) == VK_SUCCESS, "Failed to create Vulkan Logical Device!");
        FL_INFO("Created Vulkan Logical Device!");

        vkGetDeviceQueue(s_VkDevice, s_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex, 0, &s_VkGraphicsQueue);
        vkGetDeviceQueue(s_VkDevice, s_QueueFamilyIndices.PresentationSupportedQueueFamilyIndex, 0, &s_VkPresentationQueue);

        s_SwapChain = std::make_unique<VulkanSwapChain>(s_VkPhysicalDevice, s_VkDevice);

        // Creating Command Pools
        VkCommandPoolCreateInfo vk_command_pool_create_info{};
        vk_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        vk_command_pool_create_info.queueFamilyIndex = s_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex;
        vk_command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        FL_ASSERT(vkCreateCommandPool(s_VkDevice, &vk_command_pool_create_info, nullptr, &s_VkCommandPool) == VK_SUCCESS, "Failed to create Vulkan Command Pool!");
        FL_INFO("Created Vulkan Command Pool!");

        CreateCommandBuffers();
    }

    VkCommandBuffer VulkanRenderer::BeginFrame()
    {
        VkResult result = s_SwapChain->AcquireNextImage();
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            s_SwapChain->Invalidate();
            return VK_NULL_HANDLE;
        }
        s_ImageIndex = s_SwapChain->GetAcquiredImageIndex();

        // Reset Current Command Buffer
        vkResetCommandBuffer(s_VkCommandBuffers[s_CurrentFrame], 0);

        // Begin Recording Current Command Buffer
        VkCommandBufferBeginInfo vk_command_buffer_begin_info{};
        vk_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vk_command_buffer_begin_info.flags = 0;
        vk_command_buffer_begin_info.pInheritanceInfo = nullptr;

        FL_ASSERT(vkBeginCommandBuffer(s_VkCommandBuffers[s_CurrentFrame], &vk_command_buffer_begin_info) == VK_SUCCESS, "Failed to begin Vulkan Command Buffer recording!");
        return s_VkCommandBuffers[s_CurrentFrame];
    }

    void VulkanRenderer::EndFrame()
    {
        // End Recording Command Buffer
        FL_ASSERT(vkEndCommandBuffer(s_VkCommandBuffers[s_CurrentFrame]) == VK_SUCCESS, "Failed to record Vulkan Command Buffer!");

        VkResult queuePresentStatus = s_SwapChain->SubmitCommandBuffer(&s_VkCommandBuffers[s_CurrentFrame]);
        if (queuePresentStatus == VK_ERROR_OUT_OF_DATE_KHR)
            s_SwapChain->Invalidate();

        s_CurrentFrame = (s_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanRenderer::BeginRenderPass()
    {
        VkRenderPassBeginInfo vk_render_pass_begin_info{};
        vk_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        vk_render_pass_begin_info.renderPass = s_SwapChain->GetRenderPass();
        vk_render_pass_begin_info.framebuffer = s_SwapChain->GetFramebuffer(s_ImageIndex);
        vk_render_pass_begin_info.renderArea.offset = { 0, 0 };
        vk_render_pass_begin_info.renderArea.extent = s_SwapChain->GetExtent2D();

        std::array<VkClearValue, 2> vk_clear_values{};
        vk_clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        vk_clear_values[1].depthStencil = { 1.0f, 0 };

        vk_render_pass_begin_info.clearValueCount = static_cast<uint32_t>(vk_clear_values.size());
        vk_render_pass_begin_info.pClearValues = vk_clear_values.data();

        vkCmdBeginRenderPass(s_VkCommandBuffers[s_CurrentFrame], &vk_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderer::EndRenderPass()
    {
        vkCmdEndRenderPass(s_VkCommandBuffers[s_CurrentFrame]);
    }

    void VulkanRenderer::CreateCommandBuffers()
    {
        s_VkCommandBuffers.resize(s_SwapChain->GetSwapChainImageCount());

        VkCommandBufferAllocateInfo vk_command_buffer_allocate_info{};
        vk_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        vk_command_buffer_allocate_info.commandPool = s_VkCommandPool;
        vk_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        vk_command_buffer_allocate_info.commandBufferCount = (uint32_t)s_VkCommandBuffers.size();

        FL_ASSERT(vkAllocateCommandBuffers(s_VkDevice, &vk_command_buffer_allocate_info, s_VkCommandBuffers.data()) == VK_SUCCESS, "Failed to allocate Command Buffers!");
    }

    bool VulkanRenderer::HasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkFormat VulkanRenderer::GetSupportedFormat(const std::vector<VkFormat>& candidateFormats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
    {
        for (const auto& format : candidateFormats)
        {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(s_VkPhysicalDevice, format, &properties);

            if ((tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
                ||
                (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags))
                return format;
        }
        FL_ERROR("Couldn't find supported format!");
        return VK_FORMAT_UNDEFINED;
    }

    VkFormat VulkanRenderer::GetDepthFormat()
    {
        return GetSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    void VulkanRenderer::BeginSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer)
    {
        VkCommandBufferAllocateInfo vk_command_buffer_allocate_info{};
        vk_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        vk_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        vk_command_buffer_allocate_info.commandPool = s_VkCommandPool;
        vk_command_buffer_allocate_info.commandBufferCount = 1;

        vkAllocateCommandBuffers(s_VkDevice, &vk_command_buffer_allocate_info, &commandBuffer);

        VkCommandBufferBeginInfo vk_command_buffer_begin_info{};
        vk_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vk_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &vk_command_buffer_begin_info);
    }

    void VulkanRenderer::EndSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);
        VkSubmitInfo vk_submit_info{};
        vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        vk_submit_info.commandBufferCount = 1;
        vk_submit_info.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(s_VkGraphicsQueue, 1, &vk_submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(s_VkGraphicsQueue);

        vkFreeCommandBuffers(s_VkDevice, s_VkCommandPool, 1, &commandBuffer);
    }

    VkPhysicalDevice VulkanRenderer::GetValidVkPhysicalDevice(const std::vector<VkPhysicalDevice>& vk_physical_devices)
    {
        for (auto vk_device : vk_physical_devices)
        {
            QueueFamilyIndices indices;
            indices = GetQueueFamilyIndices(vk_device);

            uint32_t extensionCount = 0;
            vkEnumerateDeviceExtensionProperties(vk_device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> available_extensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(vk_device, nullptr, &extensionCount, available_extensions.data());

            std::set<std::string> required_extensions(s_VkDeviceExtensions.begin(), s_VkDeviceExtensions.end());
            for (const auto& extension : available_extensions)
                required_extensions.erase(extension.extensionName);
            bool found_required_extensions = required_extensions.empty();
            bool is_swap_chain_adequate = false;

            if (found_required_extensions)
            {
                SwapChainDetails vk_swap_chain_details = GetSwapChainDetails(vk_device);
                is_swap_chain_adequate = (!vk_swap_chain_details.SurfaceFormats.empty()) && (!vk_swap_chain_details.PresentationModes.empty());
            }

            VkPhysicalDeviceFeatures supportedFeatures;
            vkGetPhysicalDeviceFeatures(vk_device, &supportedFeatures);

            bool is_physical_device_valid = indices.GraphicsSupportedQueueFamilyIndex.has_value()
                && indices.PresentationSupportedQueueFamilyIndex.has_value()
                && found_required_extensions
                && is_swap_chain_adequate
                && supportedFeatures.samplerAnisotropy;

            if (is_physical_device_valid)
                return vk_device;
        }
        return VK_NULL_HANDLE;
    }

    uint32_t VulkanRenderer::GetValidMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags vk_memory_property_flags)
    {
        VkPhysicalDeviceMemoryProperties vk_physical_device_mem_properties;
        vkGetPhysicalDeviceMemoryProperties(s_VkPhysicalDevice, &vk_physical_device_mem_properties);

        for (uint32_t i = 0; i < vk_physical_device_mem_properties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (vk_physical_device_mem_properties.memoryTypes[i].propertyFlags & vk_memory_property_flags) == vk_memory_property_flags)
                return i;
        }
        FL_ERROR("Failed to find valid memory type!");
        return 0;
    }

    void VulkanRenderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
    {
        VkCommandBuffer commandBuffer;
        BeginSingleTimeCommandBuffer(commandBuffer);
        VkBufferCopy vk_buffer_copy_info{};
        vk_buffer_copy_info.srcOffset = 0;
        vk_buffer_copy_info.dstOffset = 0;
        vk_buffer_copy_info.size = bufferSize;

        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &vk_buffer_copy_info);
        EndSingleTimeCommandBuffer(commandBuffer);
    }

    VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char>& compiledShaderCode)
    {
        VkShaderModuleCreateInfo vk_shader_module_create_info{};
        vk_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vk_shader_module_create_info.codeSize = compiledShaderCode.size();
        vk_shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(compiledShaderCode.data());

        VkShaderModule shaderModule;
        FL_ASSERT(vkCreateShaderModule(s_VkDevice, &vk_shader_module_create_info, nullptr, &shaderModule) == VK_SUCCESS, "Failed to create shader module!");
        FL_INFO("Created Vulkan Shader Module!");
        return shaderModule;
    }

    SwapChainDetails VulkanRenderer::GetSwapChainDetails(VkPhysicalDevice vk_device)
    {
        SwapChainDetails vk_swap_chain_details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_device, s_VkSurface, &vk_swap_chain_details.SurfaceCapabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(vk_device, s_VkSurface, &formatCount, nullptr);

        if (formatCount)
        {
            vk_swap_chain_details.SurfaceFormats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(vk_device, s_VkSurface, &formatCount, vk_swap_chain_details.SurfaceFormats.data());
        }

        uint32_t presentationModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(vk_device, s_VkSurface, &presentationModeCount, nullptr);

        if (presentationModeCount)
        {
            vk_swap_chain_details.PresentationModes.resize(presentationModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(vk_device, s_VkSurface, &presentationModeCount, vk_swap_chain_details.PresentationModes.data());
        }

        return vk_swap_chain_details;
    }

    VkSurfaceFormatKHR VulkanRenderer::SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
    {
        for (const auto& format : available_formats)
        {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;
        }
        // Implement choosing of the next best format after sRGB B8G8A8 format
        return available_formats[0];
    }

    VkPresentModeKHR VulkanRenderer::SelectSwapPresentationMode(const std::vector<VkPresentModeKHR>& available_presentation_modes)
    {
        for (const auto& mode : available_presentation_modes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return mode;
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanRenderer::SelectSwapExtent(const VkSurfaceCapabilitiesKHR& surface_capabilities)
    {
        if (surface_capabilities.currentExtent.width != UINT32_MAX)
            return surface_capabilities.currentExtent;
        else
        {
            int width, height;
            glfwGetFramebufferSize(s_UserGLFWwindow, &width, &height);

            VkExtent2D actual_extent = {
                static_cast<uint32_t>(width), static_cast<uint32_t>(height)
            };

            actual_extent.width = std::clamp(actual_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
            actual_extent.height = std::clamp(actual_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
            return actual_extent;
        }
    }

    std::vector<VkDeviceQueueCreateInfo> VulkanRenderer::CreateDeviceQueueInfos(const std::set<uint32_t>& uniqueQueueFamilyIndices)
    {
        float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> vk_device_queue_create_infos;
        for (uint32_t uniqueQueueFamilyIndex : uniqueQueueFamilyIndices)
        {
            VkDeviceQueueCreateInfo vk_queue_create_info{};
            vk_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            vk_queue_create_info.queueFamilyIndex = uniqueQueueFamilyIndex;
            vk_queue_create_info.queueCount = 1;
            vk_queue_create_info.pQueuePriorities = &queuePriority;
            vk_device_queue_create_infos.push_back(vk_queue_create_info);
        }
        return vk_device_queue_create_infos;
    }

    VulkanRenderer::QueueFamilyIndices VulkanRenderer::GetQueueFamilyIndices(VkPhysicalDevice vk_device)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(vk_device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> vk_queue_family_props(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(vk_device, &queueFamilyCount, vk_queue_family_props.data());

        uint32_t index = 0;
        QueueFamilyIndices indices;
        for (const auto& vk_queue : vk_queue_family_props)
        {
            if (vk_queue.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.GraphicsSupportedQueueFamilyIndex = index;

            VkBool32 is_presentation_supported = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(vk_device, index, s_VkSurface, &is_presentation_supported);

            if (is_presentation_supported)
                indices.PresentationSupportedQueueFamilyIndex = index;

            if (indices.GraphicsSupportedQueueFamilyIndex.has_value() && indices.PresentationSupportedQueueFamilyIndex.has_value())
                break;
            index++;
        }
        return indices;
    }

    bool VulkanRenderer::CheckValidationLayerSupport()
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> available_layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, available_layers.data());

        for (const char* layerName : s_ValidationLayers)
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

    VkResult VulkanRenderer::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger)
    {
        auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (vkCreateDebugUtilsMessengerEXT)
            return vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
        else
            FL_ERROR("Failed to load function 'vkCreateDebugUtilsMessengerEXT'!");
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void VulkanRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
    {
        auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (vkDestroyDebugUtilsMessengerEXT)
            vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
        else
            FL_ERROR("Failed to load function 'vkDestroyDebugUtilsMessengerEXT'!");
    }

    void VulkanRenderer::CleanUp()
    {
        vkDeviceWaitIdle(s_VkDevice);

        vkDestroyCommandPool(s_VkDevice, s_VkCommandPool, nullptr);
        FL_INFO("Destroyed Vulkan Command Pool!");

        vkDestroyDevice(s_VkDevice, nullptr);
        FL_INFO("Destroyed Vulkan Logical Device!");

        if (s_EnableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(s_VkInstance, s_VkDebugMessenger, nullptr);
            FL_INFO("Destroyed Vulkan Debug Messenger!");
        }

        vkDestroySurfaceKHR(s_VkInstance, s_VkSurface, nullptr);
        FL_INFO("Destroyed Vulkan Window Surface!");
        vkDestroyInstance(s_VkInstance, nullptr);
        FL_INFO("Destroyed Vulkan Instance!");
    }
}
