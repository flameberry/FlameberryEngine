#pragma once

#include "Asset/Asset.h"
#include "__Material.h"
#include "Texture2D.h"

namespace Flameberry {

    struct MaterialStructGPURepresentation
    {
        glm::vec3 Albedo;
        float Roughness, Metallic;
        float TextureMapEnabled, NormalMapEnabled, RoughnessMapEnabled, AmbientOcclusionMapEnabled, MetallicMapEnabled;
    };

    // This class is basically a wrapper for the `__Material` class to add utilities for using Materials for Meshes
    class MaterialAsset : public Asset
    {
    public:
        MaterialAsset(const std::string& name, const Ref<__Material>& material);

        Ref<__Material> GetUnderlyingMaterial() { return m_MaterialRef; }

        // Getters
        std::string GetName() const { return m_Name; }
        glm::vec3   GetAlbedo() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().Albedo; }
        float       GetRoughness() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().Roughness; }
        float       GetMetallic() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().Metallic; }
        bool        IsAlbedoMapEnabled() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().TextureMapEnabled; }
        bool        IsNormalMapEnabled() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().NormalMapEnabled; }
        bool        IsRoughnessMapEnabled() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().RoughnessMapEnabled; }
        bool        IsAmbientOcclusionMapEnabled() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().AmbientOcclusionMapEnabled; }
        bool        IsMetallicMapEnabled() const { return m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().MetallicMapEnabled; }

        // Setters
        void SetName(const char* name) { m_Name = name; }
        void SetAlbedo(const glm::vec3& albedo) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().Albedo = albedo; }
        void SetRoughness(float roughness) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().Roughness = roughness; }
        void SetMetallic(float metallic) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().Metallic = metallic; }

        void SetAlbedoMapEnabled(bool value) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().TextureMapEnabled = value; }
        void SetNormalMapEnabled(bool value) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().NormalMapEnabled = value; }
        void SetRoughnessMapEnabled(bool value) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().RoughnessMapEnabled = value; }
        void SetAmbientOcclusionMapEnabled(bool value) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().AmbientOcclusionMapEnabled = value; }
        void SetMetallicMapEnabled(bool value) { m_MaterialRef->GetUniformDataReferenceAs<MaterialStructGPURepresentation>().MetallicMapEnabled = value; }

        void SetAlbedoMap(const Ref<Texture2D>& map);
        void SetNormalMap(const Ref<Texture2D>& map);
        void SetRoughnessMap(const Ref<Texture2D>& map);
        void SetAmbientOcclusionMap(const Ref<Texture2D>& map);
        void SetMetallicMap(const Ref<Texture2D>& map);

        AssetType GetAssetType() const override { return AssetType::Material; }
        static constexpr AssetType GetStaticAssetType() { return AssetType::Material; }
    private:
        std::string m_Name;
        // This is the core material which has a reference to the actual shader and stores the DescriptorSets and PushConstantData
        Ref<__Material> m_MaterialRef;

        // These textures are later then sent to the underlying `__Material` ref, i.e. `m_MaterialRef`
        Ref<Texture2D> m_AlbedoMap, m_NormalMap, m_RoughnessMap, m_AmbientOcclusionMap, m_MetallicMap;

        friend class MaterialAssetSerializer;
    };

    class MaterialAssetSerializer
    {
    public:
        static void Serialize(const Ref<MaterialAsset>& material, const char* path);
        static Ref<MaterialAsset> Deserialize(const char* path);
    };

}