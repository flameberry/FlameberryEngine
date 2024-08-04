#pragma once

#include "Asset/Asset.h"
#include "Material.h"
#include "Texture2D.h"

namespace Flameberry {

	struct MaterialStructGPURepresentation
	{
		glm::vec3 Albedo;
		float Roughness, Metallic;
		FBoolean UseAlbedoMap, UseNormalMap, UseRoughnessMap, UseAmbientMap, UseMetallicMap;
	};

	// This class is basically a wrapper for the `Material` class to add utilities for using Materials for Meshes
	class MaterialAsset : public Asset
	{
	public:
		MaterialAsset(const std::string& name);

		Ref<Material> GetUnderlyingMaterial() { return m_MaterialRef; }

		// Getters
		std::string GetName() const { return m_Name; }
		glm::vec3 GetAlbedo() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().Albedo; }
		float GetRoughness() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().Roughness; }
		float GetMetallic() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().Metallic; }
		bool IsUsingAlbedoMap() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().UseAlbedoMap; }
		bool IsUsingNormalMap() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().UseNormalMap; }
		bool IsUsingRoughnessMap() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().UseRoughnessMap; }
		bool IsUsingAmbientMap() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().UseAmbientMap; }
		bool IsUsingMetallicMap() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().UseMetallicMap; }

		// Setters
		void SetName(const char* name) { m_Name = name; }
		void SetAlbedo(const glm::vec3& albedo) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().Albedo = albedo; }
		void SetRoughness(float roughness) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().Roughness = roughness; }
		void SetMetallic(float metallic) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().Metallic = metallic; }

		void SetUseAlbedoMap(bool value) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().UseAlbedoMap = (FBoolean)value; }
		void SetUseNormalMap(bool value) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().UseNormalMap = (FBoolean)value; }
		void SetUseRoughnessMap(bool value) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().UseRoughnessMap = (FBoolean)value; }
		void SetUseAmbientMap(bool value) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().UseAmbientMap = (FBoolean)value; }
		void SetUseMetallicMap(bool value) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().UseMetallicMap = (FBoolean)value; }

		void SetAlbedoMap(AssetHandle handle);
		void SetNormalMap(AssetHandle handle);
		void SetRoughnessMap(AssetHandle handle);
		void SetAmbientMap(AssetHandle handle);
		void SetMetallicMap(AssetHandle handle);

		FBY_DECLARE_ASSET_TYPE(AssetType::Material);

	protected:
		MaterialStructGPURepresentation& GetMaterialDataRef() { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>(); }

	private:
		std::string m_Name;
		// This is the core material which has a reference to the actual shader and stores the DescriptorSets and PushConstantData
		Ref<Material> m_MaterialRef;

		// These textures are later then sent to the underlying `__Material` ref, i.e. `m_MaterialRef`
		AssetHandle m_AlbedoMap, m_NormalMap, m_RoughnessMap, m_AmbientMap, m_MetallicMap;

		friend class MaterialAssetSerializer;
		friend class MaterialEditorPanel;
	};

	class MaterialAssetSerializer
	{
	public:
		static void Serialize(const Ref<MaterialAsset>& material, const std::filesystem::path& path);
		static Ref<MaterialAsset> Deserialize(const std::filesystem::path& path);
	};

} // namespace Flameberry
