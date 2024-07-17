#include "MaterialAsset.h"

#include <fstream>
#include <glm/gtc/type_ptr.hpp>

#include "Core/YamlUtils.h"
#include "ShaderLibrary.h"
#include "Asset/AssetManager.h"

namespace Flameberry {

	MaterialAsset::MaterialAsset(const std::string& name)
		: m_Name(name), m_MaterialRef(CreateRef<Material>(ShaderLibrary::Get("PBR"))) // TODO: This should be changed immediately
	{
		// This is here to remind the developer to update the GPU representation of the Material Struct when it is updated in the shader
		FBY_ASSERT(sizeof(MaterialStructGPURepresentation) == m_MaterialRef->GetUniformDataSize(), "The GPU Representation of Material has a size ({}) which is not the same as actual Uniform size ({})", sizeof(MaterialStructGPURepresentation), m_MaterialRef->GetUniformDataSize());

		// This is very frustrating to do, otherwise Vulkan complains about no vkUpdateDescriptorSet called on certain bindings
		m_MaterialRef->Set("u_AlbedoMapSampler", Texture2D::GetEmptyImage(), Texture2D::GetDefaultSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_MaterialRef->Set("u_NormalMapSampler", Texture2D::GetEmptyImage(), Texture2D::GetDefaultSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_MaterialRef->Set("u_RoughnessMapSampler", Texture2D::GetEmptyImage(), Texture2D::GetDefaultSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_MaterialRef->Set("u_AmbientMapSampler", Texture2D::GetEmptyImage(), Texture2D::GetDefaultSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_MaterialRef->Set("u_MetallicMapSampler", Texture2D::GetEmptyImage(), Texture2D::GetDefaultSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void MaterialAsset::SetAlbedoMap(AssetHandle handle)
	{
		m_AlbedoMap = handle;
		Ref<Texture2D> map = AssetManager::GetAsset<Texture2D>(handle);
		m_MaterialRef->Set("u_AlbedoMapSampler", map);
	}

	void MaterialAsset::SetNormalMap(AssetHandle handle)
	{
		m_NormalMap = handle;
		Ref<Texture2D> map = AssetManager::GetAsset<Texture2D>(handle);
		m_MaterialRef->Set("u_NormalMapSampler", map);
	}

	void MaterialAsset::SetRoughnessMap(AssetHandle handle)
	{
		m_RoughnessMap = handle;
		Ref<Texture2D> map = AssetManager::GetAsset<Texture2D>(handle);
		m_MaterialRef->Set("u_RoughnessMapSampler", map);
	}

	void MaterialAsset::SetAmbientMap(AssetHandle handle)
	{
		m_AmbientMap = handle;
		Ref<Texture2D> map = AssetManager::GetAsset<Texture2D>(handle);
		m_MaterialRef->Set("u_AmbientMapSampler", map);
	}

	void MaterialAsset::SetMetallicMap(AssetHandle handle)
	{
		m_MetallicMap = handle;
		Ref<Texture2D> map = AssetManager::GetAsset<Texture2D>(handle);
		m_MaterialRef->Set("u_MetallicMapSampler", map);
	}

	void MaterialAssetSerializer::Serialize(const Ref<MaterialAsset>& materialAsset, const std::filesystem::path& path)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Handle" << YAML::Value << materialAsset->Handle;
		out << YAML::Key << "Name" << YAML::Value << materialAsset->m_Name;

		const float* albedo = materialAsset->m_MaterialRef->GetArray<float>("u_Albedo");
		out << YAML::Key << "Albedo" << YAML::Value << glm::vec3(albedo[0], albedo[1], albedo[2]);

		out << YAML::Key << "Roughness" << YAML::Value << materialAsset->m_MaterialRef->Get<float>("u_Roughness");
		out << YAML::Key << "Metallic" << YAML::Value << materialAsset->m_MaterialRef->Get<float>("u_Metallic");
		out << YAML::Key << "UseAlbedoMap" << YAML::Value << (bool)materialAsset->m_MaterialRef->Get<uint32_t>("u_UseAlbedoMap");
		out << YAML::Key << "AlbedoMap" << YAML::Value << materialAsset->m_AlbedoMap;
		out << YAML::Key << "UseNormalMap" << YAML::Value << (bool)materialAsset->m_MaterialRef->Get<uint32_t>("u_UseNormalMap");
		out << YAML::Key << "NormalMap" << YAML::Value << materialAsset->m_NormalMap;
		out << YAML::Key << "UseRoughnessMap" << YAML::Value << (bool)materialAsset->m_MaterialRef->Get<uint32_t>("u_UseRoughnessMap");
		out << YAML::Key << "RoughnessMap" << YAML::Value << materialAsset->m_RoughnessMap;
		out << YAML::Key << "UseAmbientMap" << YAML::Value << (bool)materialAsset->m_MaterialRef->Get<uint32_t>("u_UseAmbientMap");
		out << YAML::Key << "AmbientMap" << YAML::Value << materialAsset->m_AmbientMap;
		out << YAML::Key << "UseMetallicMap" << YAML::Value << (bool)materialAsset->m_MaterialRef->Get<uint32_t>("u_UseMetallicMap");
		out << YAML::Key << "MetallicMap" << YAML::Value << materialAsset->m_MetallicMap;
		out << YAML::EndMap;

		std::ofstream fout(path);
		fout << out.c_str();
	}

	Ref<MaterialAsset> MaterialAssetSerializer::Deserialize(const std::filesystem::path& path)
	{
		std::ifstream in(path);
		std::stringstream ss;
		ss << in.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		FBY_ASSERT(data, "Failed to load Flameberry Material from path: {}", path);

		Ref<MaterialAsset> materialAsset = CreateRef<MaterialAsset>(data["Name"].as<std::string>());
		materialAsset->Handle = data["Handle"].as<UUID>();

		materialAsset->SetAlbedo(data["Albedo"].as<glm::vec3>());
		materialAsset->SetRoughness(data["Roughness"].as<float>());
		materialAsset->SetMetallic(data["Metallic"].as<float>());

		materialAsset->SetUseAlbedoMap(data["UseAlbedoMap"].as<bool>());
		materialAsset->SetUseNormalMap(data["UseNormalMap"].as<bool>());
		materialAsset->SetUseRoughnessMap(data["UseRoughnessMap"].as<bool>());
		materialAsset->SetUseAmbientMap(data["UseAmbientMap"].as<bool>());
		materialAsset->SetUseMetallicMap(data["UseMetallicMap"].as<bool>());

		// TODO: Batch update the descriptor set of `m_MaterialRef`
		if (auto mapHandle = data["AlbedoMap"].as<AssetHandle>())
			materialAsset->SetAlbedoMap(mapHandle);

		if (auto mapHandle = data["NormalMap"].as<AssetHandle>())
			materialAsset->SetNormalMap(mapHandle);

		if (auto mapHandle = data["RoughnessMap"].as<AssetHandle>())
			materialAsset->SetRoughnessMap(mapHandle);

		if (auto mapHandle = data["AmbientMap"].as<AssetHandle>())
			materialAsset->SetAmbientMap(mapHandle);

		if (auto mapHandle = data["MetallicMap"].as<AssetHandle>())
			materialAsset->SetMetallicMap(mapHandle);

		return materialAsset;
	}

} // namespace Flameberry
