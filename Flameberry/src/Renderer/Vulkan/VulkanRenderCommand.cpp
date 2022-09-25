#include "VulkanRenderCommand.h"

#include <fstream>

#include "Core/Core.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

namespace Flameberry {
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