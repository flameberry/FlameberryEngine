#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <array>
#include <set>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Core/optional.h"

#define MAX_QUADS 10000
#define MAX_VERTICES 4 * MAX_QUADS
#define MAX_INDICES 6 * MAX_QUADS

namespace Flameberry {
    struct VulkanVertex
    {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 textureUV;

        VulkanVertex()
            : position(0.0f), color(1.0f)
        {
        }

        VulkanVertex(const glm::vec3& position, glm::vec4& color)
            : position(position), color(color)
        {
        }

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription vk_vertex_input_binding_description{};
            vk_vertex_input_binding_description.binding = 0;
            vk_vertex_input_binding_description.stride = sizeof(VulkanVertex);
            vk_vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return vk_vertex_input_binding_description;
        }

        static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 3> vk_vertex_attribute_descriptions{};

            vk_vertex_attribute_descriptions[0].binding = 0;
            vk_vertex_attribute_descriptions[0].location = 0;
            vk_vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            vk_vertex_attribute_descriptions[0].offset = offsetof(VulkanVertex, position);

            vk_vertex_attribute_descriptions[1].binding = 0;
            vk_vertex_attribute_descriptions[1].location = 1;
            vk_vertex_attribute_descriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            vk_vertex_attribute_descriptions[1].offset = offsetof(VulkanVertex, color);

            vk_vertex_attribute_descriptions[2].binding = 0;
            vk_vertex_attribute_descriptions[2].location = 2;
            vk_vertex_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            vk_vertex_attribute_descriptions[2].offset = offsetof(VulkanVertex, textureUV);

            return vk_vertex_attribute_descriptions;
        }
    };

    struct QuadCreateInfo
    {
        glm::vec2* position;
        glm::vec2* dimensions;
        glm::vec4* color;
    };

    struct UniformBufferObject
    {
        glm::mat4 ModelMatrix;
        glm::mat4 ViewMatrix;
        glm::mat4 ProjectionMatrix;
    };

    class VulkanRenderer
    {
    public:
        /// It needs the main GLFWwindow* created using glfwCreateWindow() function
        static void Init(GLFWwindow* window);
        static void RenderFrame();
        static void CleanUp();
    private:
        struct QueueFamilyIndices
        {
            flame::optional<uint32_t> GraphicsSupportedQueueFamilyIndex;
            flame::optional<uint32_t> PresentationSupportedQueueFamilyIndex;
        };
        struct SwapChainDetails
        {
            VkSurfaceCapabilitiesKHR        SurfaceCapabilities;
            std::vector<VkSurfaceFormatKHR> SurfaceFormats;
            std::vector<VkPresentModeKHR>   PresentationModes;
        };
    private:
        static void               CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        static void               TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        static void               BeginSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer);
        static void               EndSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer);
        static void               CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        static void               CreateTextureImage();
        static void               CreateTextureImageView();
        static void               CreateTextureSampler();
        static void               CreateSwapChain();
        static void               InvalidateSwapChain();
        static void               CreateGraphicsPipeline();
        static void               CreateFramebuffers();
        static void               CreateUniformBuffers();
        static void               CreateDescriptorPool();
        static void               CreateDescriptorSets();
        static void               CreateCommandBuffers();
        static void               CreateBuffer(VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer& buffer, VkDeviceMemory& deviceMemory);
        static void               CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);
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
        static void               GetQuadVertices(std::array<VulkanVertex, 4>* vertices, const QuadCreateInfo& quadCreateInfo);

        static std::vector<VkDeviceQueueCreateInfo> CreateDeviceQueueInfos(const std::set<uint32_t>& uniqueQueueFamilyIndices);
    private:
        static std::vector<char>  LoadCompiledShaderCode(const std::string& filePath);
    private:
        static VkInstance                   s_VkInstance;
        static VkDebugUtilsMessengerEXT     s_VkDebugMessenger;
        static VkPhysicalDevice             s_VkPhysicalDevice;
        static VkDevice                     s_VkDevice;
        static QueueFamilyIndices           s_QueueFamilyIndices;
        static VkQueue                      s_VkGraphicsQueue;
        static VkQueue                      s_VkPresentationQueue;
        static VkSurfaceKHR                 s_VkSurface;
        static VkSwapchainKHR               s_VkSwapChain;
        static std::vector<VkImage>         s_VkSwapChainImages;
        static VkFormat                     s_VkSwapChainImageFormat;
        static VkExtent2D                   s_VkSwapChainExtent2D;
        static std::vector<VkImageView>     s_VkSwapChainImageViews;
        static VkRenderPass                 s_VkRenderPass;
        static VkPipeline                   s_VkGraphicsPipeline;
        static std::vector<VkFramebuffer>   s_VkSwapChainFramebuffers;
        static VkCommandPool                s_VkCommandPool;
        static std::vector<VkCommandBuffer> s_VkCommandBuffers;
        static VkDescriptorPool             s_VkDescriptorPool;
        static std::vector<VkDescriptorSet> s_VkDescriptorSets;
        static VkBuffer                     s_VkVertexBuffer;
        static VkDeviceMemory               s_VkVertexBufferDeviceMemory;
        static VkBuffer                     s_VkIndexBuffer;
        static VkDeviceMemory               s_VkIndexBufferDeviceMemory;
        static std::vector<VkBuffer>        s_VkUniformBuffers;
        static std::vector<VkDeviceMemory>  s_VkUniformBuffersDeviceMemory;
        static std::vector<VkSemaphore>     s_ImageAvailableSemaphores;
        static std::vector<VkSemaphore>     s_RenderFinishedSemaphores;
        static std::vector<VkFence>         s_InFlightFences;
        static std::vector<VkFence>         s_ImagesInFlight;
        static VkDescriptorSetLayout        s_VkDescriptorSetLayout;
        static VkPipelineLayout             s_VkPipelineLayout;
        static std::vector<const char*>     s_ValidationLayers;
        static bool                         s_EnableValidationLayers;
        static std::vector<const char*>     s_VkDeviceExtensions;
        static size_t                       s_CurrentFrame;
        static uint32_t                     s_MinImageCount;
    private:
        static VkImage s_VkTextureImage;
        static VkDeviceMemory s_VkTextureImageDeviceMemory;
        static VkImageView s_VkTextureImageView;
        static VkSampler s_VkTextureSampler;
    private:
        static GLFWwindow* s_UserGLFWwindow;
        static uint32_t s_Indices[MAX_INDICES];
    private:
        constexpr static uint32_t s_MAX_FRAMES_IN_FLIGHT = 2;
    };
}