#include "MeshImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Core/Timer.h"
#include "Asset/AssetManager.h"
#include "Asset/EditorAssetManager.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/MaterialAsset.h"

namespace Flameberry {

	Ref<StaticMesh> MeshImporter::ImportMesh(AssetHandle handle, const AssetMetadata& metadata)
	{
		return LoadMesh(metadata.FilePath);
	}

	// Returns Flameberry Material Asset Handle
	static AssetHandle ProcessAndLoadMaterial(aiMaterial* material, const std::filesystem::path& path)
	{
		aiString name;
		material->Get(AI_MATKEY_NAME, name);

		std::string matName(name.C_Str());
		if (matName.empty())
			matName = "<unnamed>";

		Ref<MaterialAsset> materialAsset = CreateRef<MaterialAsset>(matName);

		aiColor3D albedo;
		material->Get(AI_MATKEY_COLOR_DIFFUSE, albedo);
		float roughness, metallic;
		material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
		material->Get(AI_MATKEY_METALLIC_FACTOR, metallic);

		materialAsset->SetAlbedo({ albedo.r, albedo.g, albedo.b });
		materialAsset->SetRoughness(roughness);
		materialAsset->SetMetallic(metallic);

		aiString albedoMap, normalMap, roughnessMap, ambientMap, metallicMap;
		aiReturn result;
		result = material->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), albedoMap);
		materialAsset->SetUseAlbedoMap(result == AI_SUCCESS);

		if (path.extension().string() == ".obj")
			result = material->Get(AI_MATKEY_TEXTURE_DISPLACEMENT(0), normalMap);
		else
			result = material->Get(AI_MATKEY_TEXTURE_NORMALS(0), normalMap);
		materialAsset->SetUseNormalMap(result == AI_SUCCESS);

		result = material->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &roughnessMap);
		materialAsset->SetUseRoughnessMap(result == AI_SUCCESS);

		result = material->Get(AI_MATKEY_TEXTURE_AMBIENT(0), ambientMap);
		materialAsset->SetUseAmbientMap(result == AI_SUCCESS);

		result = material->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &metallicMap);
		materialAsset->SetUseMetallicMap(result == AI_SUCCESS);

		std::string albedoStr(albedoMap.C_Str()), normalStr(normalMap.C_Str()), roughnessStr(roughnessMap.C_Str()), ambientStr(ambientMap.C_Str()), metallicStr(metallicMap.C_Str());

		if (materialAsset->IsUsingAlbedoMap())
		{
			std::replace(albedoStr.begin(), albedoStr.end(), '\\', '/');

			AssetHandle handle = AssetManager::As<EditorAssetManager>()->ImportAsset(path.parent_path() / albedoStr);
			materialAsset->SetAlbedoMap(handle);
		}

		if (materialAsset->IsUsingNormalMap())
		{
			std::replace(normalStr.begin(), normalStr.end(), '\\', '/');

			AssetHandle handle = AssetManager::As<EditorAssetManager>()->ImportAsset(path.parent_path() / normalStr);
			materialAsset->SetNormalMap(handle);
		}

		if (materialAsset->IsUsingRoughnessMap())
		{
			std::replace(roughnessStr.begin(), roughnessStr.end(), '\\', '/');

			AssetHandle handle = AssetManager::As<EditorAssetManager>()->ImportAsset(path.parent_path() / roughnessStr);
			materialAsset->SetRoughnessMap(handle);
		}

		if (materialAsset->IsUsingMetallicMap())
		{
			std::replace(metallicStr.begin(), metallicStr.end(), '\\', '/');

			AssetHandle handle = AssetManager::As<EditorAssetManager>()->ImportAsset(path.parent_path() / metallicStr);
			materialAsset->SetMetallicMap(handle);
		}

		if (materialAsset->IsUsingAmbientMap())
		{
			std::replace(ambientStr.begin(), ambientStr.end(), '\\', '/');

			AssetHandle handle = AssetManager::As<EditorAssetManager>()->ImportAsset(path.parent_path() / ambientStr);
			materialAsset->SetAmbientMap(handle);
		}

		AssetManager::As<EditorAssetManager>()->AddMemoryOnlyAsset(materialAsset);
		return materialAsset->Handle;
	}

	static void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::vector<MeshVertex>& refVertices, std::vector<uint32_t>& refIndices, std::vector<SubMesh>& refSubMeshes, std::vector<AssetHandle>& refMatHandles)
	{
		uint32_t indexBase = refVertices.size();

		for (uint32_t i = 0; i < mesh->mNumVertices; i++)
		{
			glm::vec2 textureCoords = mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);
			glm::vec3 tangent = mesh->HasTangentsAndBitangents() ? glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z) : glm::vec3(0.0f);
			glm::vec3 bitangent = mesh->HasTangentsAndBitangents() ? glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z) : glm::vec3(0.0f);
			refVertices.emplace_back(
				glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z), // Position
				glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z),	 // Normals
				textureCoords,																 // TextureCoords
				tangent,																	 // Tangent
				bitangent																	 // BiTangent
			);
		}

		uint32_t indexOffset = refIndices.size();
		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (uint32_t j = 0; j < face.mNumIndices; j++)
				refIndices.push_back(indexBase + face.mIndices[j]); // TODO: Make this efficient
		}

		// Add submesh
		refSubMeshes.emplace_back(SubMesh{
			refMatHandles[mesh->mMaterialIndex],
			indexOffset,
			(uint32_t)refIndices.size() - indexOffset,
			AABB(glm::vec3(mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z), glm::vec3(mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z)) });
	}

	static void ProcessNode(aiNode* node, const aiScene* scene, std::vector<MeshVertex>& refVertices, std::vector<uint32_t>& refIndices, std::vector<SubMesh>& refSubMeshes, std::vector<AssetHandle>& refMatHandles)
	{
		// Process all the node's meshes (if any)
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			ProcessMesh(mesh, scene, refVertices, refIndices, refSubMeshes, refMatHandles);
		}

		// Then do the same for each of its children
		for (uint32_t i = 0; i < node->mNumChildren; i++)
			ProcessNode(node->mChildren[i], scene, refVertices, refIndices, refSubMeshes, refMatHandles);
	}

	Ref<StaticMesh> MeshImporter::LoadMesh(const std::filesystem::path& path)
	{
		FBY_SCOPED_TIMER("Load_Model_Assimp");

		// Create an instance of the Importer class
		Assimp::Importer importer;

		// And have it read the given file with some example postprocessing
		// Usually - if speed is not the most important aspect for you - you'll
		// probably to request more postprocessing than we do in this example.
		const aiScene* scene = importer.ReadFile(path.string(),
			aiProcessPreset_TargetRealtime_Fast
				// aiProcessPreset_TargetRealtime_Quality
				| aiProcess_FlipUVs
				| aiProcess_GenBoundingBoxes);

		// If the import failed, report it
		if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
		{
			FBY_ERROR("{}", importer.GetErrorString());
			return nullptr;
		}

		// Process Scene
		std::vector<MeshVertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<SubMesh> submeshes;
		std::vector<AssetHandle> materialHandles;

		FBY_LOG("Number of materials in mesh: {}: {}", path, scene->mNumMaterials);

		// Load Materials
		for (uint32_t i = 0; i < scene->mNumMaterials; i++)
		{
			auto* mat = scene->mMaterials[i];
			materialHandles.emplace_back(ProcessAndLoadMaterial(mat, path));
		}

		// Load Meshes
		ProcessNode(scene->mRootNode, scene, vertices, indices, submeshes, materialHandles);

		Ref<Buffer> vertexBuffer, indexBuffer;

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
			RenderCommand::CopyBuffer(stagingBuffer.GetVulkanBuffer(), vertexBuffer->GetVulkanBuffer(), bufferSize);
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
			RenderCommand::CopyBuffer(stagingBuffer.GetVulkanBuffer(), indexBuffer->GetVulkanBuffer(), bufferSize);
		}

		FBY_INFO("Loaded Model: '{}': Vertices: {}, Indices: {}", path, vertices.size(), indices.size());
		return CreateRef<StaticMesh>(vertexBuffer, indexBuffer, submeshes);
	}

} // namespace Flameberry
