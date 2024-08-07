#include "VulkanContext.h"

#include <set>
#include <string>

#include "Core/Core.h"

#include "RenderCommand.h"
#include "SwapChain.h"

namespace Flameberry {

	const std::vector<const char*> VulkanContext::s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

	std::vector<const char*> VulkanContext::s_VulkanDeviceExtensions = {
		"VK_KHR_portability_subset",
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

	VkPhysicalDevice VulkanContext::GetValidPhysicalDevice(const std::vector<VkPhysicalDevice>& vk_physical_devices, VkSurfaceKHR surface)
	{
		for (auto vk_device : vk_physical_devices)
		{
			QueueFamilyIndices indices = RenderCommand::GetQueueFamilyIndices(vk_device, surface);

			uint32_t extensionCount = 0;
			vkEnumerateDeviceExtensionProperties(vk_device, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> available_extensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(vk_device, nullptr, &extensionCount, available_extensions.data());

			std::set<std::string> required_extensions(s_VulkanDeviceExtensions.begin(), s_VulkanDeviceExtensions.end());

			FBY_INFO("Available Extensions:");
			for (const auto& extension : available_extensions)
			{
				FBY_INFO("\t{}", extension.extensionName);
				required_extensions.erase(extension.extensionName);
			}
			bool found_required_extensions = required_extensions.empty();
			bool is_swap_chain_adequate = false;

			if (found_required_extensions)
			{
				SwapChainDetails vk_swap_chain_details = RenderCommand::GetSwapChainDetails(vk_device, surface);
				is_swap_chain_adequate = (!vk_swap_chain_details.SurfaceFormats.empty()) && (!vk_swap_chain_details.PresentationModes.empty());
			}
			else
			{
				FBY_ERROR("Failed to find the following extension(s):");
				for (const auto& name : required_extensions)
					FBY_ERROR("\t{}", name);
			}

			VkPhysicalDeviceFeatures supportedFeatures;
			vkGetPhysicalDeviceFeatures(vk_device, &supportedFeatures);

			VkPhysicalDeviceFeatures2 deviceFeatures2{};
			deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

			VkPhysicalDeviceVulkan12Features vulkan12Features{};
			vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

			deviceFeatures2.pNext = &vulkan12Features;
			vkGetPhysicalDeviceFeatures2(vk_device, &deviceFeatures2);

			bool is_physical_device_valid = indices.GraphicsAndComputeSupportedQueueFamilyIndex != -1
				&& indices.PresentationSupportedQueueFamilyIndex != -1
				&& found_required_extensions
				&& is_swap_chain_adequate
				&& supportedFeatures.samplerAnisotropy
				&& supportedFeatures.sampleRateShading
				&& supportedFeatures.fillModeNonSolid
				&& supportedFeatures.tessellationShader
				&& supportedFeatures.depthClamp
				&& supportedFeatures.shaderStorageImageArrayDynamicIndexing
				&& vulkan12Features.descriptorIndexing;

			if (is_physical_device_valid)
				return vk_device;
		}
		FBY_ASSERT(0, "Failed to find valid physical device!");
		return VK_NULL_HANDLE;
	}

} // namespace Flameberry
