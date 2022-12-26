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
    VulkanRenderer::VulkanRenderer(VulkanWindow* pWindow)
        : m_VulkanContext(pWindow)
    {
        VulkanContext::SetCurrentContext(&m_VulkanContext);

        VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VkPhysicalDevice physicalDevice = VulkanContext::GetPhysicalDevice();

        m_SwapChain = std::make_unique<VulkanSwapChain>(pWindow->GetWindowSurface());

        VulkanContext::GetCurrentDevice()->AllocateCommandBuffers(m_SwapChain->GetSwapChainImageCount());
    }

    VkCommandBuffer VulkanRenderer::BeginFrame()
    {
        VkResult result = m_SwapChain->AcquireNextImage();
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_SwapChain->Invalidate();
            return VK_NULL_HANDLE;
        }
        m_ImageIndex = m_SwapChain->GetAcquiredImageIndex();

        // Begin Recording Current Command Buffer
        // VkCommandBufferBeginInfo vk_command_buffer_begin_info{};
        // vk_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        // vk_command_buffer_begin_info.flags = 0;
        // vk_command_buffer_begin_info.pInheritanceInfo = nullptr;

        // FL_ASSERT(vkBeginCommandBuffer(m_VkCommandBuffers[m_CurrentFrame], &vk_command_buffer_begin_info) == VK_SUCCESS, "Failed to begin Vulkan Command Buffer recording!");
        // return m_VkCommandBuffers[m_CurrentFrame];

        const auto& device = VulkanContext::GetCurrentDevice();
        device->ResetCommandBuffer(m_CurrentFrame);
        device->BeginCommandBuffer(m_CurrentFrame);
        return device->GetCommandBuffer(m_CurrentFrame);
    }

    void VulkanRenderer::EndFrame()
    {
        // End Recording Command Buffer
        // FL_ASSERT(vkEndCommandBuffer(m_VkCommandBuffers[m_CurrentFrame]) == VK_SUCCESS, "Failed to record Vulkan Command Buffer!");

        const auto& device = VulkanContext::GetCurrentDevice();
        device->EndCommandBuffer(m_CurrentFrame);

        VkResult queuePresentStatus = m_SwapChain->SubmitCommandBuffer(device->GetCommandBuffer(m_CurrentFrame));
        if (queuePresentStatus == VK_ERROR_OUT_OF_DATE_KHR)
            m_SwapChain->Invalidate();

        m_CurrentFrame = (m_CurrentFrame + 1) % VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanRenderer::BeginRenderPass()
    {
        VkRenderPassBeginInfo vk_render_pass_begin_info{};
        vk_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        vk_render_pass_begin_info.renderPass = m_SwapChain->GetRenderPass();
        vk_render_pass_begin_info.framebuffer = m_SwapChain->GetFramebuffer(m_ImageIndex);
        vk_render_pass_begin_info.renderArea.offset = { 0, 0 };
        vk_render_pass_begin_info.renderArea.extent = m_SwapChain->GetExtent2D();

        std::array<VkClearValue, 2> vk_clear_values{};
        vk_clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        vk_clear_values[1].depthStencil = { 1.0f, 0 };

        vk_render_pass_begin_info.clearValueCount = static_cast<uint32_t>(vk_clear_values.size());
        vk_render_pass_begin_info.pClearValues = vk_clear_values.data();

        const auto& device = VulkanContext::GetCurrentDevice();
        vkCmdBeginRenderPass(device->GetCommandBuffer(m_CurrentFrame), &vk_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderer::EndRenderPass()
    {
        const auto& device = VulkanContext::GetCurrentDevice();
        vkCmdEndRenderPass(device->GetCommandBuffer(m_CurrentFrame));
    }

    VulkanRenderer::~VulkanRenderer()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDeviceWaitIdle(device);
    }

    // void VulkanRenderer::Init(GLFWwindow* window)
    // {
    //     FL_ASSERT(window, "GLFW Window provided to VulkanRenderer is null!");
    //     m_UserGLFWwindow = window;

        // if (m_EnableValidationLayers)
        //     FL_ASSERT(VulkanRenderCommand::CheckValidationLayerSupport(), "Validation Layers are requested but not available!");

        // // Creating Vulkan Instance
        // VkApplicationInfo vk_app_info{};
        // vk_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        // vk_app_info.pApplicationName = "Flameberry";
        // vk_app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        // vk_app_info.pEngineName = "No Engine";
        // vk_app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        // vk_app_info.apiVersion = VK_API_VERSION_1_2;

        // VkInstanceCreateInfo vk_create_info{};
        // vk_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        // vk_create_info.pApplicationInfo = &vk_app_info;

        // uint32_t glfwExtensionCount = 0;
        // const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // std::vector<const char*> vk_extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // vk_extensions.push_back("VK_KHR_portability_enumeration");
        // if (m_EnableValidationLayers)
        //     vk_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        // vk_create_info.enabledExtensionCount = static_cast<uint32_t>(vk_extensions.size());
        // vk_create_info.ppEnabledExtensionNames = vk_extensions.data();
        // vk_create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        // VkDebugUtilsMessengerCreateInfoEXT vk_debug_create_info{};

        // if (m_EnableValidationLayers)
        // {
        //     vk_create_info.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        //     vk_create_info.ppEnabledLayerNames = m_ValidationLayers.data();

        //     vk_debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        //     vk_debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        //     vk_debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        //     vk_debug_create_info.pfnUserCallback = vk_debug_callback;

        //     vk_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&vk_debug_create_info;
        // }
        // else
        // {
        //     vk_create_info.enabledLayerCount = 0;
        //     vk_create_info.pNext = nullptr;
        // }

        // FL_ASSERT(vkCreateInstance(&vk_create_info, nullptr, &m_VkInstance) == VK_SUCCESS, "Failed to create Vulkan Instance!");
        // FL_INFO("Created Vulkan Instance!");

        // if (m_EnableValidationLayers)
        // {
        //     // Creating Vulkan Debug Messenger
        //     VkDebugUtilsMessengerCreateInfoEXT vk_debug_messenger_create_info{};
        //     vk_debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        //     vk_debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        //     vk_debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        //     vk_debug_messenger_create_info.pfnUserCallback = vk_debug_callback;
        //     vk_debug_messenger_create_info.pUserData = nullptr;

        //     FL_ASSERT(CreateDebugUtilsMessengerEXT(m_VkInstance, &vk_debug_messenger_create_info, nullptr, &m_VkDebugMessenger) == VK_SUCCESS, "Failed to created Vulkan Debug Messenger!");
        //     FL_INFO("Created Vulkan Debug Messenger!");
        // }

        // // Creating Vulkan Window Surface
        // FL_ASSERT(glfwCreateWindowSurface(m_VkInstance, m_UserGLFWwindow, nullptr, &m_VkSurface) == VK_SUCCESS, "Failed to create window surface!");

        // // Setting up Valid Vulkan Physical Device
        // uint32_t deviceCount = 0;
        // vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, nullptr);

        // FL_ASSERT(deviceCount, "Failed to find GPUs which support Vulkan!");

        // std::vector<VkPhysicalDevice> vk_physical_devices(deviceCount);
        // vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, vk_physical_devices.data());

        // // Printing Physical Devices Names
        // std::string physical_device_list = "";
        // for (uint16_t i = 0; i < deviceCount; i++)
        // {
        //     VkPhysicalDeviceProperties vk_physical_device_props;
        //     vkGetPhysicalDeviceProperties(vk_physical_devices[i], &vk_physical_device_props);
        //     physical_device_list += vk_physical_device_props.deviceName;
        //     if (i < deviceCount - 1)
        //         physical_device_list += ", ";
        // }

        // FL_INFO("{0} Physical devices found: {1}", deviceCount, physical_device_list);

        // // Accessing the actual physical device
        // m_VkPhysicalDevice = GetValidVkPhysicalDevice(vk_physical_devices);
        // FL_ASSERT(m_VkPhysicalDevice != VK_NULL_HANDLE, "Vulkan physical device is null!");

        // VkPhysicalDeviceProperties vk_physical_device_props;
        // vkGetPhysicalDeviceProperties(m_VkPhysicalDevice, &vk_physical_device_props);
        // FL_INFO("Selected Vulkan Physical Device: {0}", vk_physical_device_props.deviceName);

        // // Getting Queue Family Indices
        // m_QueueFamilyIndices = VulkanRenderCommand::GetQueueFamilyIndices(m_VkPhysicalDevice);

        // std::vector<VkDeviceQueueCreateInfo> vk_device_queue_create_infos = CreateDeviceQueueInfos(
        //     {
        //         m_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex,
        //         m_QueueFamilyIndices.PresentationSupportedQueueFamilyIndex
        //     }
        // );

        // VkPhysicalDeviceFeatures vk_physical_device_features{};
        // vk_physical_device_features.samplerAnisotropy = VK_TRUE;

        // // Creating Vulkan Logical Device
        // VkDeviceCreateInfo vk_device_create_info{};
        // vk_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        // vk_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(vk_device_queue_create_infos.size());
        // vk_device_create_info.pQueueCreateInfos = vk_device_queue_create_infos.data();

        // vk_device_create_info.pEnabledFeatures = &vk_physical_device_features;

        // vk_device_create_info.enabledExtensionCount = static_cast<uint32_t>(s_VkDeviceExtensions.size());
        // vk_device_create_info.ppEnabledExtensionNames = s_VkDeviceExtensions.data();

        // if (m_EnableValidationLayers)
        // {
        //     vk_device_create_info.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        //     vk_device_create_info.ppEnabledLayerNames = m_ValidationLayers.data();
        // }
        // else
        // {
        //     vk_device_create_info.enabledLayerCount = 0;
        // }

        // FL_ASSERT(vkCreateDevice(m_VkPhysicalDevice, &vk_device_create_info, nullptr, &m_VkDevice) == VK_SUCCESS, "Failed to create Vulkan Logical Device!");
        // FL_INFO("Created Vulkan Logical Device!");

        // vkGetDeviceQueue(m_VkDevice, m_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex, 0, &m_VkGraphicsQueue);
        // vkGetDeviceQueue(m_VkDevice, m_QueueFamilyIndices.PresentationSupportedQueueFamilyIndex, 0, &m_VkPresentationQueue);

    //     m_SwapChain = std::make_unique<VulkanSwapChain>(m_VkPhysicalDevice, m_VkDevice);

    //     // Creating Command Pools
    //     VkCommandPoolCreateInfo vk_command_pool_create_info{};
    //     vk_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    //     vk_command_pool_create_info.queueFamilyIndex = m_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex;
    //     vk_command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    //     FL_ASSERT(vkCreateCommandPool(m_VkDevice, &vk_command_pool_create_info, nullptr, &m_VkCommandPool) == VK_SUCCESS, "Failed to create Vulkan Command Pool!");
    //     FL_INFO("Created Vulkan Command Pool!");

    //     CreateCommandBuffers();
    // }
}
