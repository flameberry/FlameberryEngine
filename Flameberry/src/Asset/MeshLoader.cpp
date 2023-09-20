#include "MeshLoader.h"

#include "Core/Timer.h"
#include "AssetManager.h"
#include "Renderer/RenderCommand.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

namespace Flameberry {
    template <typename T, typename... Rest>
    void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
        seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);
    };
}

namespace std {
    template <>
    struct hash<Flameberry::MeshVertex> {
        size_t operator()(Flameberry::MeshVertex const& vertex) const {
            size_t seed = 0;
            Flameberry::hashCombine(seed, vertex.Position, vertex.Normal, vertex.TextureUV);
            return seed;
        }
    };
}

namespace Flameberry {

    std::shared_ptr<Asset> MeshLoader::LoadMesh(const std::filesystem::path& path)
    {
        FL_SCOPED_TIMER("Load_Model");
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        std::string mtlBaseDir = path.parent_path();
        FL_ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), mtlBaseDir.c_str()), err);

        bool has_tex_coord = attrib.texcoords.size();

        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<SubMesh> submeshes;
        std::vector<AssetHandle> materialHandles;
        std::unordered_map<MeshVertex, uint32_t> uniqueVertices{};

        for (const auto& mat : materials) {
            auto materialAsset = std::make_shared<Material>();
            materialAsset->SetName(mat.name.c_str());
            materialAsset->SetAlbedo({ mat.diffuse[0], mat.diffuse[1], mat.diffuse[2] });
            materialAsset->SetRoughness(mat.roughness);
            materialAsset->SetMetallic(mat.metallic);

            materialAsset->SetAlbedoMapEnabled(!mat.diffuse_texname.empty());
            if (materialAsset->IsAlbedoMapEnabled())
                materialAsset->SetAlbedoMap(AssetManager::TryGetOrLoadAsset<Texture2D>(mtlBaseDir + "/" + mat.diffuse_texname));

            materialAsset->SetNormalMapEnabled(!mat.displacement_texname.empty());
            if (materialAsset->IsNormalMapEnabled())
                materialAsset->SetNormalMap(AssetManager::TryGetOrLoadAsset<Texture2D>(mtlBaseDir + "/" + mat.displacement_texname));

            materialAsset->SetRoughnessMapEnabled(!mat.roughness_texname.empty());
            if (materialAsset->IsRoughnessMapEnabled())
                materialAsset->SetRoughnessMap(AssetManager::TryGetOrLoadAsset<Texture2D>(mtlBaseDir + "/" + mat.roughness_texname));

            materialAsset->SetMetallicMapEnabled(!mat.metallic_texname.empty());
            if (materialAsset->IsMetallicMapEnabled())
                materialAsset->SetMetallicMap(AssetManager::TryGetOrLoadAsset<Texture2D>(mtlBaseDir + "/" + mat.metallic_texname));

            materialAsset->SetAmbientOcclusionMapEnabled(!mat.ambient_texname.empty());
            if (materialAsset->IsAmbientOcclusionMapEnabled())
                materialAsset->SetAmbientOcclusionMap(AssetManager::TryGetOrLoadAsset<Texture2D>(mtlBaseDir + "/" + mat.ambient_texname));

            materialAsset->Update();

            AssetManager::RegisterAsset(materialAsset);
            materialHandles.emplace_back(materialAsset->Handle);
        }

        for (const auto& shape : shapes)
        {
            int i = 0;
            uint32_t indexCount = indices.size();

            MeshVertex triangleVertices[3] = {};

            for (const auto& index : shape.mesh.indices)
            {
                triangleVertices[i % 3].Position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                triangleVertices[i % 3].Normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };

                if (has_tex_coord) {
                    triangleVertices[i % 3].TextureUV = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                }

                i++;

                if (i % 3 == 0)
                {
                    glm::vec3 vertex0 = triangleVertices[0].Position;
                    glm::vec3 vertex1 = triangleVertices[1].Position;
                    glm::vec3 vertex2 = triangleVertices[2].Position;

                    glm::vec3 tangent, bitangent;

                    glm::vec3 edge1 = vertex1 - vertex0;
                    glm::vec3 edge2 = vertex2 - vertex0;

                    glm::vec2 uv0 = triangleVertices[0].TextureUV;
                    glm::vec2 uv1 = triangleVertices[1].TextureUV;
                    glm::vec2 uv2 = triangleVertices[2].TextureUV;

                    glm::vec2 deltaUV1 = uv1 - uv0;
                    glm::vec2 deltaUV2 = uv2 - uv0;

                    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

                    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

                    bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
                    bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
                    bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

                    for (uint16_t j = 0; j < 3; j++)
                    {
                        auto& vertex = triangleVertices[j];

                        // Tangents and Bitangents
                        if (uniqueVertices.count(vertex) == 0) {
                            vertex.Tangent = tangent;
                            vertex.BiTangent = bitangent;

                            uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                            vertices.push_back(vertex);
                        }
                        else
                        {
                            uint32_t index = uniqueVertices[vertex];
                            vertices[index].Tangent += tangent;
                            vertices[index].BiTangent += bitangent;
                        }
                        indices.push_back(uniqueVertices[vertex]);
                    }
                }
            }

            auto& submesh = submeshes.emplace_back();
            submesh.IndexCount = indices.size() - indexCount;
            submesh.IndexOffset = indexCount;

            if (shape.mesh.material_ids[0] != -1)
                submesh.MaterialHandle = materialHandles[shape.mesh.material_ids[0]];
        }

        std::shared_ptr<Buffer> vertexBuffer, indexBuffer;

        {
            // Creating Vertex Buffer
            VkDeviceSize bufferSize = sizeof(MeshVertex) * vertices.size();

            BufferSpecification stagingBufferSpec;
            stagingBufferSpec.InstanceCount = 1;
            stagingBufferSpec.InstanceSize = bufferSize;
            stagingBufferSpec.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            stagingBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            Buffer stagingBuffer(stagingBufferSpec);

            stagingBuffer.MapMemory(bufferSize);
            stagingBuffer.WriteToBuffer(vertices.data(), bufferSize, 0);
            stagingBuffer.UnmapMemory();

            BufferSpecification vertexBufferSpec;
            vertexBufferSpec.InstanceCount = 1;
            vertexBufferSpec.InstanceSize = bufferSize;
            vertexBufferSpec.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            vertexBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            vertexBuffer = std::make_unique<Buffer>(vertexBufferSpec);
            RenderCommand::CopyBuffer(stagingBuffer.GetBuffer(), vertexBuffer->GetBuffer(), bufferSize);
        }

        {
            // Creating Index Buffer
            VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();
            BufferSpecification stagingBufferSpec;

            stagingBufferSpec.InstanceCount = 1;
            stagingBufferSpec.InstanceSize = bufferSize;
            stagingBufferSpec.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            stagingBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            Buffer stagingBuffer(stagingBufferSpec);

            stagingBuffer.MapMemory(bufferSize);
            stagingBuffer.WriteToBuffer(indices.data(), bufferSize, 0);
            stagingBuffer.UnmapMemory();

            BufferSpecification indexBufferSpec;
            indexBufferSpec.InstanceCount = 1;
            indexBufferSpec.InstanceSize = bufferSize;
            indexBufferSpec.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            indexBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            indexBuffer = std::make_unique<Buffer>(indexBufferSpec);
            RenderCommand::CopyBuffer(stagingBuffer.GetBuffer(), indexBuffer->GetBuffer(), bufferSize);
        }

        auto meshAsset = std::make_shared<StaticMesh>(vertexBuffer, indexBuffer, submeshes);

        // Set Asset Class Variables
        meshAsset->FilePath = path;
        meshAsset->SizeInBytesOnCPU = sizeof(StaticMesh);
        meshAsset->SizeInBytesOnGPU = vertices.size() * sizeof(MeshVertex) + indices.size() * sizeof(uint32_t);

        FL_INFO("Loaded Model: '{0}': Vertices: {1}, Indices: {2}", path, vertices.size(), indices.size());
        return meshAsset;
    }

}