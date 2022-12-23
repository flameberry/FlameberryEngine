#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <set>

#include "Renderer/PerspectiveCamera.h"
#include "VulkanVertex.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "VulkanSwapChain.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Core/optional.h"

#define MAX_QUADS 1000000
#define MAX_VERTICES 4 * MAX_QUADS
#define MAX_INDICES 6 * MAX_QUADS

#define MAX_FRAMES_IN_FLIGHT 2

namespace Flameberry {
    struct QuadCreateInfo
    {
        glm::vec2* position;
        glm::vec2* dimensions;
        glm::vec4* color;
    };

    struct CameraUniformBufferObject { glm::mat4 ViewProjectionMatrix; };
    struct ModelMatrixPushConstantData { glm::mat4 ModelMatrix; };

    struct SwapChainDetails
    {
        VkSurfaceCapabilitiesKHR        SurfaceCapabilities;
        std::vector<VkSurfaceFormatKHR> SurfaceFormats;
        std::vector<VkPresentModeKHR>   PresentationModes;
    };

    class VulkanRenderer
    {
    public:
        /// It needs the main GLFWwindow* created using glfwCreateWindow() function
        static void Init(GLFWwindow* window);
        [[nodiscard]] static VkCommandBuffer BeginFrame();
        static void EndFrame();
        static void BeginRenderPass();
        static void EndRenderPass();
        static void CleanUp();
    private:
        struct QueueFamilyIndices
        {
            flame::optional<uint32_t> GraphicsSupportedQueueFamilyIndex;
            flame::optional<uint32_t> PresentationSupportedQueueFamilyIndex;
        };
    public:
        static VkDevice& GetDevice() { return s_VkDevice; }
        static VkQueue GetGraphicsQueue() { return s_VkGraphicsQueue; }
        static VkQueue GetPresentationQueue() { return s_VkPresentationQueue; }
        static VkSurfaceKHR GetSurface() { return s_VkSurface; }
        static VkRenderPass GetRenderPass() { return s_SwapChain->GetRenderPass(); }
        static uint32_t GetCurrentFrameIndex() { return s_CurrentFrame; }
        static const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(s_VkPhysicalDevice, &properties);
            return properties;
        }

        static void               CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);
        static VkExtent2D         GetSwapChainExtent2D() { return s_SwapChain->GetExtent2D(); };
        static void               BeginSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer);
        static void               EndSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer);
        static void               CreateCommandBuffers();
        static VkPhysicalDevice   GetValidVkPhysicalDevice(const std::vector<VkPhysicalDevice>& vk_physical_devices);
        static bool               CheckValidationLayerSupport();
        static VkResult           CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger);
        static void               DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator);
        static QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice vk_device);
        static SwapChainDetails   GetSwapChainDetails(VkPhysicalDevice vk_device);
        static VkSurfaceFormatKHR SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
        static VkPresentModeKHR   SelectSwapPresentationMode(const std::vector<VkPresentModeKHR>& available_presentation_modes);
        static VkExtent2D         SelectSwapExtent(const VkSurfaceCapabilitiesKHR& surface_capabilities);
        static VkShaderModule     CreateShaderModule(const std::vector<char>& compiledShaderCode);
        static uint32_t           GetValidMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags vk_memory_property_flags);
        static VkFormat           GetSupportedFormat(const std::vector<VkFormat>& candidateFormats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
        static VkFormat           GetDepthFormat();
        static bool               HasStencilComponent(VkFormat format);

        static std::vector<VkDeviceQueueCreateInfo> CreateDeviceQueueInfos(const std::set<uint32_t>& uniqueQueueFamilyIndices);
    private:
        static VkInstance                   s_VkInstance;
        static VkDebugUtilsMessengerEXT     s_VkDebugMessenger;
        static VkPhysicalDevice             s_VkPhysicalDevice;
        static VkDevice                     s_VkDevice;
        static QueueFamilyIndices           s_QueueFamilyIndices;
        static VkQueue                      s_VkGraphicsQueue;
        static VkQueue                      s_VkPresentationQueue;
        static VkSurfaceKHR                 s_VkSurface;
        static VkCommandPool                s_VkCommandPool;
        static std::vector<VkCommandBuffer> s_VkCommandBuffers;
        static std::vector<const char*>     s_ValidationLayers;
        static bool                         s_EnableValidationLayers;
        static std::vector<const char*>     s_VkDeviceExtensions;
        static size_t                       s_CurrentFrame;
        static uint32_t                     s_ImageIndex;

        static std::unique_ptr<VulkanSwapChain> s_SwapChain;
    private:
        static GLFWwindow* s_UserGLFWwindow;
    };
}
