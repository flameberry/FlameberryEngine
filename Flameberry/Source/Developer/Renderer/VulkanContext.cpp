#include "VulkanContext.h"

#include <set>
#include <map>
#include <string>

#include "Core/Core.h"

#include "RenderCommand.h"
#include "SwapChain.h"

namespace Flameberry {

	const std::vector<const char*> VulkanContext::s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

	std::vector<const char*> VulkanContext::s_VulkanDeviceExtensions = {
#ifdef FBY_PLATFORM_MACOS
		"VK_KHR_portability_subset",
#endif
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MULTIVIEW_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
	};

	VulkanContext* VulkanContext::s_CurrentContext = nullptr;
#ifdef FBY_DEBUG
	bool VulkanContext::s_EnableValidationLayers = true;
#else
	bool VulkanContext::s_EnableValidationLayers = true;
#endif

	VulkanContext::VulkanContext(VulkanWindow* pWindow)
		: m_Window(pWindow)
	{
		FBY_ASSERT(pWindow->GetGLFWwindow(), "Null Window given to Vulkan Context!");

		if (s_EnableValidationLayers)
		{
			bool isSupported = RenderCommand::CheckValidationLayerSupport(s_ValidationLayers);
			if (!isSupported)
				FBY_ERROR("Validation Layers are not supported!");
		}

		m_VulkanInstance = CreateRef<VulkanInstance>();

		pWindow->CreateVulkanWindowSurface(m_VulkanInstance->GetVulkanInstance());

		// Setting up Valid Vulkan Physical Device
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_VulkanInstance->GetVulkanInstance(), &deviceCount, nullptr);
		FBY_ASSERT(deviceCount, "Failed to find GPUs which support Vulkan!");

		std::vector<VkPhysicalDevice> vk_physical_devices(deviceCount);
		vkEnumeratePhysicalDevices(m_VulkanInstance->GetVulkanInstance(), &deviceCount, vk_physical_devices.data());

		// Print Physical Devices Names
		std::string physical_device_list = "";
		for (uint16_t i = 0; i < deviceCount; i++)
		{
			VkPhysicalDeviceProperties vk_physical_device_props;
			vkGetPhysicalDeviceProperties(vk_physical_devices[i], &vk_physical_device_props);
			physical_device_list += vk_physical_device_props.deviceName;
			if (i < deviceCount - 1)
				physical_device_list += ", ";
		}

		FBY_TRACE("{} Physical devices found: {}", deviceCount, physical_device_list);

		// Accessing the actual physical device
		m_VkPhysicalDevice = GetValidPhysicalDevice(vk_physical_devices, pWindow->GetWindowSurface());
		FBY_ASSERT(m_VkPhysicalDevice != VK_NULL_HANDLE, "Vulkan physical device is null!");

		VkPhysicalDeviceProperties vk_physical_device_props;
		vkGetPhysicalDeviceProperties(m_VkPhysicalDevice, &vk_physical_device_props);
		FBY_INFO("Selected Vulkan Physical Device: {}", vk_physical_device_props.deviceName);

		m_VulkanDevice = CreateRef<VulkanDevice>(m_VkPhysicalDevice, pWindow);

		// const uint32_t maxDescSets = 300;
		const uint32_t maxDescSets = 500;

		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 * 8 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxDescSets },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 50 }
		};
		m_GlobalDescriptorPool = CreateRef<DescriptorPool>(m_VulkanDevice->GetVulkanDevice(), poolSizes, maxDescSets);

#if (defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
		// SRS - on macOS set environment variable to configure MoltenVK for using Metal argument buffers (needed for descriptor indexing)
		//     - MoltenVK supports Metal argument buffers on macOS, iOS possible in future (see https://github.com/KhronosGroup/MoltenVK/issues/1651)
		setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "1", 1);
#endif
	}

	VulkanContext::~VulkanContext()
	{
		m_Window->DestroyVulkanWindowSurface(m_VulkanInstance->GetVulkanInstance());
	}

	VkPhysicalDevice VulkanContext::GetValidPhysicalDevice(const std::vector<VkPhysicalDevice>& physicalDeviceList, VkSurfaceKHR surface)
	{
		std::map<int, VkPhysicalDevice> physicalDeviceMap;

		for (auto physicalDevice : physicalDeviceList)
		{
			QueueFamilyIndices indices = RenderCommand::GetQueueFamilyIndices(physicalDevice, surface);

			uint32_t extensionCount = 0;
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

			std::set<std::string> requiredExtensions(s_VulkanDeviceExtensions.begin(), s_VulkanDeviceExtensions.end());

			FBY_INFO("Available Extensions:");
			for (const auto& extension : availableExtensions)
			{
				FBY_INFO("\t{}", extension.extensionName);
				requiredExtensions.erase(extension.extensionName);
			}
			bool foundRequiredExtensions = requiredExtensions.empty();
			bool isSwapchainAdequate = false;

			if (foundRequiredExtensions)
			{
				SwapChainDetails vulkanSwapchainDetails = RenderCommand::GetSwapChainDetails(physicalDevice, surface);
				isSwapchainAdequate = (!vulkanSwapchainDetails.SurfaceFormats.empty()) && (!vulkanSwapchainDetails.PresentationModes.empty());
			}
			else
			{
				FBY_ERROR("Failed to find the following extension(s):");
				for (const auto& name : requiredExtensions)
					FBY_ERROR("\t{}", name);
			}

			VkPhysicalDeviceFeatures supportedFeatures;
			vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

			VkPhysicalDeviceFeatures2 deviceFeatures2{};
			deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

			VkPhysicalDeviceVulkan12Features vulkan12Features{};
			vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

			deviceFeatures2.pNext = &vulkan12Features;
			vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

			bool isPhysicalDeviceValid = indices.GraphicsQueueFamilyIndex != -1
				&& indices.ComputeQueueFamilyIndex!= -1
				&& indices.PresentationSupportedQueueFamilyIndex != -1
				&& foundRequiredExtensions
				&& isSwapchainAdequate
				&& supportedFeatures.samplerAnisotropy
				&& supportedFeatures.sampleRateShading
				&& supportedFeatures.fillModeNonSolid
				&& supportedFeatures.tessellationShader
				&& supportedFeatures.depthClamp
				&& supportedFeatures.shaderStorageImageArrayDynamicIndexing
				&& vulkan12Features.descriptorIndexing;

			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

			int score = 0;

			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				score -= 1000;

			if (isPhysicalDeviceValid)
				physicalDeviceMap[score] = physicalDevice;
		}
		FBY_ASSERT(physicalDeviceMap.size(), "Failed to find valid physical device!");
		return physicalDeviceMap.rbegin()->second;
	}

} // namespace Flameberry
