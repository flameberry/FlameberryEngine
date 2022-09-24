// #include "VulkanTexture.h"

// #include "Core/Log.h"

// #define STB_IMAGE_IMPLEMENTATION
// #include <stb_image/stb_image.h>

// namespace Flameberry {
//     VulkanTexture::VulkanTexture()
//     {
//     }

//     VulkanTexture::~VulkanTexture()
//     {
//     }

//     void VulkanTexture::CreateTextureImage(VkDevice device)
//     {
//         int width, height, channels;
//         stbi_uc* pixels = stbi_load(FL_PROJECT_DIR"SandboxApp/assets/textures/StoneIdol.jpg", &width, &height, &channels, STBI_rgb_alpha);
//         FL_ASSERT(pixels, "Texture pixels are empty!");
//         VkDeviceSize imageSize = 4 * width * height;

//         VkBuffer stagingBuffer;
//         VkDeviceMemory stagingBufferMemory;

//         CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

//         void* data;
//         vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
//         memcpy(data, pixels, static_cast<size_t>(imageSize));
//         vkUnmapMemory(device, stagingBufferMemory);

//         stbi_image_free(pixels);

//         CreateImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, s_VkTextureImage, s_VkTextureImageDeviceMemory);

//         TransitionImageLayout(m_VkTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
//         CopyBufferToImage(stagingBuffer, m_VkTextureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
//         TransitionImageLayout(m_VkTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

//         vkDestroyBuffer(device, stagingBuffer, nullptr);
//         vkFreeMemory(device, stagingBufferMemory, nullptr);
//     }
// }