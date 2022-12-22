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
    VkSwapchainKHR               VulkanRenderer::s_VkSwapChain;
    std::vector<VkImage>         VulkanRenderer::s_VkSwapChainImages;
    VkFormat                     VulkanRenderer::s_VkSwapChainImageFormat;
    VkExtent2D                   VulkanRenderer::s_VkSwapChainExtent2D;
    std::vector<VkImageView>     VulkanRenderer::s_VkSwapChainImageViews;
    VkRenderPass                 VulkanRenderer::s_VkRenderPass;
    std::vector<VkFramebuffer>   VulkanRenderer::s_VkSwapChainFramebuffers;
    VkCommandPool                VulkanRenderer::s_VkCommandPool;
    std::vector<VkCommandBuffer> VulkanRenderer::s_VkCommandBuffers;
    std::vector<VkSemaphore>     VulkanRenderer::s_ImageAvailableSemaphores;
    std::vector<VkSemaphore>     VulkanRenderer::s_RenderFinishedSemaphores;
    std::vector<VkFence>         VulkanRenderer::s_InFlightFences;
    std::vector<VkFence>         VulkanRenderer::s_ImagesInFlight;
    std::vector<const char*>     VulkanRenderer::s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
    bool                         VulkanRenderer::s_EnableValidationLayers = true;
    size_t                       VulkanRenderer::s_CurrentFrame = 0;
    uint32_t                     VulkanRenderer::s_ImageIndex;
    uint32_t                     VulkanRenderer::s_MinImageCount;

    VkImage VulkanRenderer::s_VkDepthImage;
    VkDeviceMemory VulkanRenderer::s_VkDepthImageMemory;
    VkImageView VulkanRenderer::s_VkDepthImageView;

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

        // Swap Chain Creation
        CreateSwapChain();

        // Image Views creation
        s_VkSwapChainImageViews.resize(s_VkSwapChainImages.size());

        for (uint32_t i = 0; i < s_VkSwapChainImages.size(); i++)
        {
            VkImageViewCreateInfo vk_image_view_create_info{};
            vk_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            vk_image_view_create_info.image = s_VkSwapChainImages[i];
            vk_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vk_image_view_create_info.format = s_VkSwapChainImageFormat;
            vk_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vk_image_view_create_info.subresourceRange.baseMipLevel = 0;
            vk_image_view_create_info.subresourceRange.levelCount = 1;
            vk_image_view_create_info.subresourceRange.baseArrayLayer = 0;
            vk_image_view_create_info.subresourceRange.layerCount = 1;

            FL_ASSERT(vkCreateImageView(s_VkDevice, &vk_image_view_create_info, nullptr, &s_VkSwapChainImageViews[i]) == VK_SUCCESS, "Failed to create Vulkan Image View!");
        }
        FL_INFO("Created {0} Vulkan Image Views!", s_VkSwapChainImageViews.size());

        // Creating Render Passes
        VkSubpassDependency vk_subpass_dependency{};
        vk_subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        vk_subpass_dependency.dstSubpass = 0;
        vk_subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        vk_subpass_dependency.srcAccessMask = 0;
        vk_subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        vk_subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkAttachmentDescription vk_color_attachment_description{};
        vk_color_attachment_description.format = s_VkSwapChainImageFormat;
        vk_color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
        vk_color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        vk_color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        vk_color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        vk_color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vk_color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vk_color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference vk_color_attachment_reference{};
        vk_color_attachment_reference.attachment = 0;
        vk_color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription vk_depth_attachment_desc{};
        vk_depth_attachment_desc.format = GetDepthFormat();
        vk_depth_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
        vk_depth_attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        vk_depth_attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vk_depth_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        vk_depth_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vk_depth_attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vk_depth_attachment_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference vk_depth_attachment_ref{};
        vk_depth_attachment_ref.attachment = 1;
        vk_depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription vk_subpass_description{};
        vk_subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        vk_subpass_description.colorAttachmentCount = 1;
        vk_subpass_description.pColorAttachments = &vk_color_attachment_reference;
        vk_subpass_description.pDepthStencilAttachment = &vk_depth_attachment_ref;

        std::array<VkAttachmentDescription, 2> attachments = { vk_color_attachment_description, vk_depth_attachment_desc };

        VkRenderPassCreateInfo vk_render_pass_create_info{};
        vk_render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        vk_render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        vk_render_pass_create_info.pAttachments = attachments.data();
        vk_render_pass_create_info.subpassCount = 1;
        vk_render_pass_create_info.pSubpasses = &vk_subpass_description;
        vk_render_pass_create_info.dependencyCount = 1;
        vk_render_pass_create_info.pDependencies = &vk_subpass_dependency;

        FL_ASSERT(vkCreateRenderPass(s_VkDevice, &vk_render_pass_create_info, nullptr, &s_VkRenderPass) == VK_SUCCESS, "Failed to create Vulkan Render Pass!");
        FL_INFO("Created Vulkan Render Pass!");

        CreateDepthResources();
        CreateFramebuffers();

        // Creating Command Pools
        VkCommandPoolCreateInfo vk_command_pool_create_info{};
        vk_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        vk_command_pool_create_info.queueFamilyIndex = s_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex;
        vk_command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        FL_ASSERT(vkCreateCommandPool(s_VkDevice, &vk_command_pool_create_info, nullptr, &s_VkCommandPool) == VK_SUCCESS, "Failed to create Vulkan Command Pool!");
        FL_INFO("Created Vulkan Command Pool!");

        CreateCommandBuffers();

        // Creating Semaphores and Fences
        s_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        s_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        s_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        s_ImagesInFlight.resize(s_VkSwapChainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo vk_semaphore_create_info{};
        vk_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo vk_fence_create_info{};
        vk_fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vk_fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            FL_ASSERT(vkCreateSemaphore(s_VkDevice, &vk_semaphore_create_info, nullptr, &s_ImageAvailableSemaphores[i]) == VK_SUCCESS, "Failed to create Image Available Semaphore!");
            FL_ASSERT(vkCreateSemaphore(s_VkDevice, &vk_semaphore_create_info, nullptr, &s_RenderFinishedSemaphores[i]) == VK_SUCCESS, "Failed to create Render Finished Semaphore!");
            FL_ASSERT(vkCreateFence(s_VkDevice, &vk_fence_create_info, nullptr, &s_InFlightFences[i]) == VK_SUCCESS, "Failed to create 'in flight Fence'!");
        }
        FL_INFO("Created {0} Image Available Semaphores, Render Finished Semaphores, and 'in flight fences'!", MAX_FRAMES_IN_FLIGHT);
    }

    VkCommandBuffer VulkanRenderer::BeginFrame()
    {
        vkWaitForFences(s_VkDevice, 1, &s_InFlightFences[s_CurrentFrame], VK_TRUE, UINT64_MAX);

        VkResult imageAcquireStatus = vkAcquireNextImageKHR(s_VkDevice, s_VkSwapChain, UINT64_MAX, s_ImageAvailableSemaphores[s_CurrentFrame], VK_NULL_HANDLE, &s_ImageIndex);
        if (imageAcquireStatus == VK_ERROR_OUT_OF_DATE_KHR)
        {
            InvalidateSwapChain();
            return VK_NULL_HANDLE;
        }

        // Check if a previous frame is using this image (i.e. there is its fence to wait on)
        if (s_ImagesInFlight[s_ImageIndex] != VK_NULL_HANDLE)
            vkWaitForFences(s_VkDevice, 1, &s_ImagesInFlight[s_ImageIndex], VK_TRUE, UINT64_MAX);

        // Mark the image as now being in use by this frame
        s_ImagesInFlight[s_ImageIndex] = s_InFlightFences[s_CurrentFrame];

        vkResetFences(s_VkDevice, 1, &s_InFlightFences[s_CurrentFrame]);

        // Reset Current Command Buffer
        vkResetCommandBuffer(s_VkCommandBuffers[s_CurrentFrame], 0);

        // Begin Recording Current Command Buffer
        VkCommandBufferBeginInfo vk_command_buffer_begin_info{};
        vk_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vk_command_buffer_begin_info.flags = 0;
        vk_command_buffer_begin_info.pInheritanceInfo = nullptr;

        FL_ASSERT(vkBeginCommandBuffer(s_VkCommandBuffers[s_CurrentFrame], &vk_command_buffer_begin_info) == VK_SUCCESS, "Failed to begin Vulkan Command Buffer recording!");

        VkRenderPassBeginInfo vk_render_pass_begin_info{};
        vk_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        vk_render_pass_begin_info.renderPass = s_VkRenderPass;
        vk_render_pass_begin_info.framebuffer = s_VkSwapChainFramebuffers[s_ImageIndex];
        vk_render_pass_begin_info.renderArea.offset = { 0, 0 };
        vk_render_pass_begin_info.renderArea.extent = s_VkSwapChainExtent2D;

        std::array<VkClearValue, 2> vk_clear_values{};
        vk_clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        vk_clear_values[1].depthStencil = { 1.0f, 0 };

        vk_render_pass_begin_info.clearValueCount = static_cast<uint32_t>(vk_clear_values.size());
        vk_render_pass_begin_info.pClearValues = vk_clear_values.data();

        vkCmdBeginRenderPass(s_VkCommandBuffers[s_CurrentFrame], &vk_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        // vkCmdBindPipeline(s_VkCommandBuffers[s_CurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, s_VkGraphicsPipeline);
        return s_VkCommandBuffers[s_CurrentFrame];
    }

    void VulkanRenderer::EndFrame()
    {
        // End Recording Command Buffer
        vkCmdEndRenderPass(s_VkCommandBuffers[s_CurrentFrame]);
        FL_ASSERT(vkEndCommandBuffer(s_VkCommandBuffers[s_CurrentFrame]) == VK_SUCCESS, "Failed to record Vulkan Command Buffer!");

        // Submit Queue
        VkSubmitInfo vk_submit_info{};
        vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore wait_semaphores[] = { s_ImageAvailableSemaphores[s_CurrentFrame] };
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        vk_submit_info.waitSemaphoreCount = 1;
        vk_submit_info.pWaitSemaphores = wait_semaphores;
        vk_submit_info.pWaitDstStageMask = wait_stages;
        vk_submit_info.commandBufferCount = 1;
        vk_submit_info.pCommandBuffers = &s_VkCommandBuffers[s_CurrentFrame];

        VkSemaphore signal_semaphores[] = { s_RenderFinishedSemaphores[s_CurrentFrame] };
        vk_submit_info.signalSemaphoreCount = 1;
        vk_submit_info.pSignalSemaphores = signal_semaphores;

        FL_ASSERT(vkQueueSubmit(s_VkGraphicsQueue, 1, &vk_submit_info, s_InFlightFences[s_CurrentFrame]) == VK_SUCCESS, "Failed to submit Graphics Queue!");

        VkPresentInfoKHR vk_present_info{};
        vk_present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        vk_present_info.waitSemaphoreCount = 1;
        vk_present_info.pWaitSemaphores = signal_semaphores;

        VkSwapchainKHR vk_swap_chains[] = { s_VkSwapChain };
        vk_present_info.swapchainCount = 1;
        vk_present_info.pSwapchains = vk_swap_chains;
        vk_present_info.pImageIndices = &s_ImageIndex;
        vk_present_info.pResults = nullptr;

        VkResult queuePresentStatus = vkQueuePresentKHR(s_VkPresentationQueue, &vk_present_info);
        if (queuePresentStatus == VK_ERROR_OUT_OF_DATE_KHR)
            InvalidateSwapChain();
        vkQueueWaitIdle(s_VkPresentationQueue);

        s_CurrentFrame = (s_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanRenderer::CreateSwapChain()
    {
        SwapChainDetails vk_swap_chain_details = GetSwapChainDetails(s_VkPhysicalDevice);
        VkSurfaceFormatKHR vk_surface_format = SelectSwapSurfaceFormat(vk_swap_chain_details.SurfaceFormats);
        VkPresentModeKHR vk_presentation_mode = SelectSwapPresentationMode(vk_swap_chain_details.PresentationModes);
        VkExtent2D vk_extent_2d = SelectSwapExtent(vk_swap_chain_details.SurfaceCapabilities);

        s_VkSwapChainImageFormat = vk_surface_format.format;
        s_VkSwapChainExtent2D = vk_extent_2d;

        uint32_t imageCount = vk_swap_chain_details.SurfaceCapabilities.minImageCount + 1;

        if ((vk_swap_chain_details.SurfaceCapabilities.maxImageCount > 0) && (imageCount > vk_swap_chain_details.SurfaceCapabilities.maxImageCount))
            imageCount = vk_swap_chain_details.SurfaceCapabilities.maxImageCount;

        VkSwapchainCreateInfoKHR vk_swap_chain_create_info{};
        vk_swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        vk_swap_chain_create_info.surface = s_VkSurface;
        vk_swap_chain_create_info.minImageCount = imageCount;
        vk_swap_chain_create_info.imageFormat = vk_surface_format.format;
        vk_swap_chain_create_info.imageColorSpace = vk_surface_format.colorSpace;
        vk_swap_chain_create_info.imageExtent = vk_extent_2d;
        vk_swap_chain_create_info.imageArrayLayers = 1;
        vk_swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        s_MinImageCount = imageCount;

        uint32_t vk_queue_indices[2] = { s_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex , s_QueueFamilyIndices.PresentationSupportedQueueFamilyIndex };

        if (s_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex != s_QueueFamilyIndices.PresentationSupportedQueueFamilyIndex)
        {
            vk_swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            vk_swap_chain_create_info.queueFamilyIndexCount = 2;
            vk_swap_chain_create_info.pQueueFamilyIndices = vk_queue_indices;
        }
        else
        {
            vk_swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vk_swap_chain_create_info.queueFamilyIndexCount = 0;
            vk_swap_chain_create_info.pQueueFamilyIndices = nullptr;
        }

        vk_swap_chain_create_info.preTransform = vk_swap_chain_details.SurfaceCapabilities.currentTransform;
        vk_swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        vk_swap_chain_create_info.presentMode = vk_presentation_mode;
        vk_swap_chain_create_info.clipped = VK_TRUE;
        vk_swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;

        FL_ASSERT(vkCreateSwapchainKHR(s_VkDevice, &vk_swap_chain_create_info, nullptr, &s_VkSwapChain) == VK_SUCCESS, "Failed to create Vulkan Swap Chain!");
        FL_INFO("Created Vulkan Swap Chain!");

        uint32_t vk_swap_chain_image_count = 0;
        vkGetSwapchainImagesKHR(s_VkDevice, s_VkSwapChain, &vk_swap_chain_image_count, nullptr);
        s_VkSwapChainImages.resize(vk_swap_chain_image_count);
        vkGetSwapchainImagesKHR(s_VkDevice, s_VkSwapChain, &vk_swap_chain_image_count, s_VkSwapChainImages.data());
    }

    void VulkanRenderer::InvalidateSwapChain()
    {
        int width = 0, height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(s_UserGLFWwindow, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(s_VkDevice);

        // CleanUp SwapChain
        for (auto framebuffer : s_VkSwapChainFramebuffers)
            vkDestroyFramebuffer(s_VkDevice, framebuffer, nullptr);

        for (auto imageView : s_VkSwapChainImageViews)
            vkDestroyImageView(s_VkDevice, imageView, nullptr);

        vkDestroySwapchainKHR(s_VkDevice, s_VkSwapChain, nullptr);

        // Create New SwapChain
        CreateSwapChain();

        // Creating Image Views
        s_VkSwapChainImageViews.resize(s_VkSwapChainImages.size());

        for (uint32_t i = 0; i < s_VkSwapChainImages.size(); i++)
        {
            VkImageViewCreateInfo vk_image_view_create_info{};
            vk_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            vk_image_view_create_info.image = s_VkSwapChainImages[i];
            vk_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vk_image_view_create_info.format = s_VkSwapChainImageFormat;
            vk_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vk_image_view_create_info.subresourceRange.baseMipLevel = 0;
            vk_image_view_create_info.subresourceRange.levelCount = 1;
            vk_image_view_create_info.subresourceRange.baseArrayLayer = 0;
            vk_image_view_create_info.subresourceRange.layerCount = 1;

            FL_ASSERT(vkCreateImageView(s_VkDevice, &vk_image_view_create_info, nullptr, &s_VkSwapChainImageViews[i]) == VK_SUCCESS, "Failed to create Vulkan Image View!");
        }

        CreateFramebuffers();
        s_ImagesInFlight.resize(s_VkSwapChainImages.size(), VK_NULL_HANDLE);
    }

    void VulkanRenderer::CreateFramebuffers()
    {
        s_VkSwapChainFramebuffers.resize(s_VkSwapChainImageViews.size());

        for (size_t i = 0; i < s_VkSwapChainImageViews.size(); i++)
        {
            std::array<VkImageView, 2> attachments = { s_VkSwapChainImageViews[i], s_VkDepthImageView };

            VkFramebufferCreateInfo vk_framebuffer_create_info{};
            vk_framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            vk_framebuffer_create_info.renderPass = s_VkRenderPass;
            vk_framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
            vk_framebuffer_create_info.pAttachments = attachments.data();
            vk_framebuffer_create_info.width = s_VkSwapChainExtent2D.width;
            vk_framebuffer_create_info.height = s_VkSwapChainExtent2D.height;
            vk_framebuffer_create_info.layers = 1;

            FL_ASSERT(vkCreateFramebuffer(s_VkDevice, &vk_framebuffer_create_info, nullptr, &s_VkSwapChainFramebuffers[i]) == VK_SUCCESS, "Failed to create Vulkan Framebuffer!");
        }
    }

    void VulkanRenderer::CreateCommandBuffers()
    {
        s_VkCommandBuffers.resize(s_VkSwapChainFramebuffers.size());

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

    void VulkanRenderer::CreateDepthResources()
    {
        VkFormat depthFormat = GetDepthFormat();
        CreateImage(s_VkSwapChainExtent2D.width, s_VkSwapChainExtent2D.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, s_VkDepthImage, s_VkDepthImageMemory);
        s_VkDepthImageView = CreateImageView(s_VkDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
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

    void VulkanRenderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
    {
        VkImageCreateInfo vk_image_create_info{};
        vk_image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        vk_image_create_info.imageType = VK_IMAGE_TYPE_2D;
        vk_image_create_info.extent.width = width;
        vk_image_create_info.extent.height = height;
        vk_image_create_info.extent.depth = 1;
        vk_image_create_info.mipLevels = 1;
        vk_image_create_info.arrayLayers = 1;
        vk_image_create_info.format = format;
        vk_image_create_info.tiling = tiling;
        vk_image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vk_image_create_info.usage = usage;
        vk_image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        vk_image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        FL_ASSERT(vkCreateImage(s_VkDevice, &vk_image_create_info, nullptr, &image) == VK_SUCCESS, "Failed to create Vulkan Texture Image!");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(s_VkDevice, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = GetValidMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

        FL_ASSERT(vkAllocateMemory(s_VkDevice, &allocInfo, nullptr, &imageMemory) == VK_SUCCESS, "Failed to allocate memory!");

        vkBindImageMemory(s_VkDevice, image, imageMemory, 0);
    }

    VkImageView VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo vk_image_view_create_info{};
        vk_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vk_image_view_create_info.image = image;
        vk_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vk_image_view_create_info.format = format;
        vk_image_view_create_info.subresourceRange.aspectMask = aspectFlags;
        vk_image_view_create_info.subresourceRange.baseMipLevel = 0;
        vk_image_view_create_info.subresourceRange.levelCount = 1;
        vk_image_view_create_info.subresourceRange.baseArrayLayer = 0;
        vk_image_view_create_info.subresourceRange.layerCount = 1;

        VkImageView vk_image_view;
        FL_ASSERT(vkCreateImageView(s_VkDevice, &vk_image_view_create_info, nullptr, &vk_image_view) == VK_SUCCESS, "Failed to create Vulkan Image View!");
        return vk_image_view;
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

    void VulkanRenderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkCommandBuffer commandBuffer;
        BeginSingleTimeCommandBuffer(commandBuffer);

        VkPipelineStageFlags sourceStageFlags;
        VkPipelineStageFlags destinationStageFlags;


        VkImageMemoryBarrier vk_image_memory_barrier{};

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            vk_image_memory_barrier.srcAccessMask = 0;
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            vk_image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
            FL_ERROR("Unsupported Image Layout Transition");

        vk_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        vk_image_memory_barrier.oldLayout = oldLayout;
        vk_image_memory_barrier.newLayout = newLayout;
        vk_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk_image_memory_barrier.image = image;
        vk_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vk_image_memory_barrier.subresourceRange.baseMipLevel = 0;
        vk_image_memory_barrier.subresourceRange.levelCount = 1;
        vk_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        vk_image_memory_barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(commandBuffer, sourceStageFlags, destinationStageFlags, 0, 0, nullptr, 0, nullptr, 1, &vk_image_memory_barrier);

        EndSingleTimeCommandBuffer(commandBuffer);
    }

    void VulkanRenderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        VkCommandBuffer commandBuffer;
        BeginSingleTimeCommandBuffer(commandBuffer);

        VkBufferImageCopy vk_buffer_image_copy_region{};
        vk_buffer_image_copy_region.bufferOffset = 0;
        vk_buffer_image_copy_region.bufferRowLength = 0;
        vk_buffer_image_copy_region.bufferImageHeight = 0;

        vk_buffer_image_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vk_buffer_image_copy_region.imageSubresource.mipLevel = 0;
        vk_buffer_image_copy_region.imageSubresource.baseArrayLayer = 0;
        vk_buffer_image_copy_region.imageSubresource.layerCount = 1;

        vk_buffer_image_copy_region.imageOffset = { 0, 0, 0 };
        vk_buffer_image_copy_region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vk_buffer_image_copy_region);

        EndSingleTimeCommandBuffer(commandBuffer);
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

    VulkanRenderer::SwapChainDetails VulkanRenderer::GetSwapChainDetails(VkPhysicalDevice vk_device)
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

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(s_VkDevice, s_ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(s_VkDevice, s_RenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(s_VkDevice, s_InFlightFences[i], nullptr);
        }
        FL_INFO("Destroyed {0} Image Available Semaphores, Render Finished Semaphores and 'in flight fences'!", MAX_FRAMES_IN_FLIGHT);

        vkDestroyCommandPool(s_VkDevice, s_VkCommandPool, nullptr);
        FL_INFO("Destroyed Vulkan Command Pool!");

        size_t frambufferCount = s_VkSwapChainFramebuffers.size();
        for (auto& framebuffer : s_VkSwapChainFramebuffers)
            vkDestroyFramebuffer(s_VkDevice, framebuffer, nullptr);
        FL_INFO("Destroyed {0} Vulkan Framebuffers!", frambufferCount);

        vkDestroyRenderPass(s_VkDevice, s_VkRenderPass, nullptr);
        FL_INFO("Destroyed Vulkan Render Pass!");

        size_t imageViewCount = s_VkSwapChainImageViews.size();
        for (auto imageView : s_VkSwapChainImageViews)
            vkDestroyImageView(s_VkDevice, imageView, nullptr);
        FL_INFO("Destroyed {0} Vulkan Image Views!", imageViewCount);

        vkDestroySwapchainKHR(s_VkDevice, s_VkSwapChain, nullptr);
        FL_INFO("Destroyed Vulkan Swap Chain!");

        vkDestroyImageView(s_VkDevice, s_VkDepthImageView, nullptr);
        vkDestroyImage(s_VkDevice, s_VkDepthImage, nullptr);
        vkFreeMemory(s_VkDevice, s_VkDepthImageMemory, nullptr);
        FL_INFO("Destroyed Vulkan Depth Image View and Image and freed Depth Image Device Memory!");

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
