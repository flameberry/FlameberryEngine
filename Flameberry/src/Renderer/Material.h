#pragma once

#include <glm/glm.hpp>
#include "Texture2D.h"

#include "Core/UUID.h"

namespace Flameberry {
    class Material : public Asset
    {
    public:
        Material();

        AssetType GetAssetType() const override { return AssetType::Material; }
        static constexpr AssetType GetStaticAssetType() { return AssetType::Material; }
        static std::shared_ptr<DescriptorSetLayout> GetLayout() { return s_CommonDescSetLayout; }
        static VkDescriptorSet GetEmptyDesciptorSet() { return s_EmptyMaterialDescSet->GetDescriptorSet(); }

        VkDescriptorSet GetDescriptorSet() const { return m_TextureMapSet->GetDescriptorSet(); }

        // Getters
        std::string GetName() const { return m_Name; }
        glm::vec3   GetAlbedo() const { return m_Albedo; }
        float       GetRoughness() const { return m_Roughness; }
        float       GetMetallic() const { return m_Metallic; }
        bool        IsAlbedoMapEnabled() const { return m_AlbedoMapEnabled; }
        bool        IsNormalMapEnabled() const { return m_NormalMapEnabled; }
        bool        IsRoughnessMapEnabled() const { return m_RoughnessMapEnabled; }
        bool        IsAmbientOcclusionMapEnabled() const { return m_AmbientOcclusionMapEnabled; }
        bool        IsMetallicMapEnabled() const { return m_MetallicMapEnabled; }

        // Setters
        void SetName(const char* name) { m_Name = name; }
        void SetAlbedo(const glm::vec3& albedo) { m_Albedo = albedo; }
        void SetRoughness(float roughness) { m_Roughness = roughness; }
        void SetMetallic(float metallic) { m_Metallic = metallic; }
        void SetAlbedoMap(const std::shared_ptr<Texture2D>& albedoMap) { m_AlbedoMap = albedoMap; }
        void SetNormalMap(const std::shared_ptr<Texture2D>& normalMap) { m_NormalMap = normalMap; }
        void SetRoughnessMap(const std::shared_ptr<Texture2D>& roughnessMap) { m_RoughnessMap = roughnessMap; }
        void SetAmbientOcclusionMap(const std::shared_ptr<Texture2D>& ambientOccMap) { m_AmbientOcclusionMap = ambientOccMap; }
        void SetMetallicMap(const std::shared_ptr<Texture2D>& metallicMap) { m_MetallicMap = metallicMap; }

        void SetAlbedoMapEnabled(bool value) { m_AlbedoMapEnabled = value; }
        void SetNormalMapEnabled(bool value) { m_NormalMapEnabled = value; }
        void SetRoughnessMapEnabled(bool value) { m_RoughnessMapEnabled = value; }
        void SetAmbientOcclusionMapEnabled(bool value) { m_AmbientOcclusionMapEnabled = value; }
        void SetMetallicMapEnabled(bool value) { m_MetallicMapEnabled = value; }

        void Update();

        static void CreateLayout(); // TODO: Make this private
    private:
        std::string m_Name = "Default_Material";
        glm::vec3 m_Albedo{ 1.0f };
        float m_Roughness = 0.2f, m_Metallic = 0.0f;
        bool m_AlbedoMapEnabled = false, m_NormalMapEnabled = false, m_RoughnessMapEnabled = false, m_AmbientOcclusionMapEnabled = false, m_MetallicMapEnabled = false;
        std::shared_ptr<Texture2D> m_AlbedoMap, m_NormalMap, m_RoughnessMap, m_AmbientOcclusionMap, m_MetallicMap;

        std::unique_ptr<DescriptorSet> m_TextureMapSet;
        static std::shared_ptr<DescriptorSetLayout> s_CommonDescSetLayout;
        static std::unique_ptr<DescriptorSet> s_EmptyMaterialDescSet;

        friend class MaterialSerializer;
        friend class MaterialEditorPanel;
    };

    class MaterialSerializer
    {
    public:
        static void Serialize(const std::shared_ptr<Material>& material, const char* path);
        static std::shared_ptr<Material> Deserialize(const char* path);
    };
}
