#include "Material.h"

#include <fstream>
#include "Core/YamlUtils.h"

#include "Asset/AssetManager.h"

namespace Flameberry {
    Ref<DescriptorSetLayout> Material::s_CommonDescSetLayout;
    std::unique_ptr<DescriptorSet> Material::s_EmptyMaterialDescSet;

    Material::Material()
    {
        if (!s_CommonDescSetLayout || !s_EmptyMaterialDescSet)
            Init();

        DescriptorSetSpecification descSetSpec;
        descSetSpec.Layout = s_CommonDescSetLayout;
        m_TextureMapSet = std::make_unique<DescriptorSet>(descSetSpec);
        
        VkDescriptorImageInfo imageInfo{
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .sampler = Texture2D::GetDefaultSampler(),
            .imageView = Texture2D::GetEmptyImageView()
        };

        for (uint8_t i = 0; i < 5; i++)
            m_TextureMapSet->WriteImage(i, imageInfo);
        m_TextureMapSet->Update();
    }

    void Material::Update()
    {
        VkDescriptorImageInfo imageInfos[5] = {};

        for (uint32_t i = 0; i < 5; i++)
        {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        if (m_AlbedoMap)
        {
            imageInfos[0].imageView = m_AlbedoMap->GetImageView();
            imageInfos[0].sampler = m_AlbedoMap->GetSampler();
            m_TextureMapSet->WriteImage(0, imageInfos[0]);
        }

        if (m_NormalMap)
        {
            imageInfos[1].imageView = m_NormalMap->GetImageView();
            imageInfos[1].sampler = m_NormalMap->GetSampler();
            m_TextureMapSet->WriteImage(1, imageInfos[1]);
        }

        if (m_RoughnessMap)
        {
            imageInfos[2].imageView = m_RoughnessMap->GetImageView();
            imageInfos[2].sampler = m_RoughnessMap->GetSampler();
            m_TextureMapSet->WriteImage(2, imageInfos[2]);
        }

        if (m_AmbientOcclusionMap)
        {
            imageInfos[3].imageView = m_AmbientOcclusionMap->GetImageView();
            imageInfos[3].sampler = m_AmbientOcclusionMap->GetSampler();
            m_TextureMapSet->WriteImage(3, imageInfos[3]);
        }

        if (m_MetallicMap)
        {
            imageInfos[4].imageView = m_MetallicMap->GetImageView();
            imageInfos[4].sampler = m_MetallicMap->GetSampler();
            m_TextureMapSet->WriteImage(4, imageInfos[4]);
        }

        m_TextureMapSet->Update();
    }

    void Material::Init()
    {
        DescriptorSetLayoutSpecification layoutSpec;
        layoutSpec.Bindings.resize(5);

        for (uint8_t i = 0; i < 5; i++)
        {
            layoutSpec.Bindings[i].binding = i;
            layoutSpec.Bindings[i].descriptorCount = 1;
            layoutSpec.Bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            layoutSpec.Bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        s_CommonDescSetLayout = CreateRef<DescriptorSetLayout>(layoutSpec);

        DescriptorSetSpecification descSetSpec;
        descSetSpec.Layout = s_CommonDescSetLayout;

        s_EmptyMaterialDescSet = std::make_unique<DescriptorSet>(descSetSpec);
        
        VkDescriptorImageInfo imageInfo{
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .sampler = Texture2D::GetDefaultSampler(),
            .imageView = Texture2D::GetEmptyImageView()
        };

        for (uint8_t i = 0; i < 5; i++)
            s_EmptyMaterialDescSet->WriteImage(i, imageInfo);
        s_EmptyMaterialDescSet->Update();
    }

    void Material::Shutdown()
    {
        s_EmptyMaterialDescSet.release();
        s_CommonDescSetLayout = nullptr; // TODO: This is the current cause of the VK_ERROR
    }

    void MaterialSerializer::Serialize(const Ref<Material>& material, const char* path)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Handle" << YAML::Value << material->Handle;
        out << YAML::Key << "Name" << YAML::Value << material->m_Name;
        out << YAML::Key << "Albedo" << YAML::Value << material->m_Albedo;
        out << YAML::Key << "Roughness" << YAML::Value << material->m_Roughness;
        out << YAML::Key << "Metallic" << YAML::Value << material->m_Metallic;
        out << YAML::Key << "TextureMapEnabled" << YAML::Value << material->m_AlbedoMapEnabled;
        out << YAML::Key << "TextureMap" << YAML::Value << (material->m_AlbedoMapEnabled ? material->m_AlbedoMap->FilePath : "");
        out << YAML::Key << "NormalMapEnabled" << YAML::Value << material->m_NormalMapEnabled;
        out << YAML::Key << "NormalMap" << YAML::Value << (material->m_NormalMapEnabled ? material->m_NormalMap->FilePath : "");
        out << YAML::Key << "RoughnessMapEnabled" << YAML::Value << material->m_RoughnessMapEnabled;
        out << YAML::Key << "RoughnessMap" << YAML::Value << (material->m_RoughnessMapEnabled ? material->m_RoughnessMap->FilePath : "");
        out << YAML::Key << "AmbientOcclusionMapEnabled" << YAML::Value << material->m_AmbientOcclusionMapEnabled;
        out << YAML::Key << "AmbientOcclusionMap" << YAML::Value << (material->m_AmbientOcclusionMapEnabled ? material->m_AmbientOcclusionMap->FilePath : "");
        out << YAML::Key << "MetallicMapEnabled" << YAML::Value << material->m_MetallicMapEnabled;
        out << YAML::Key << "MetallicMap" << YAML::Value << (material->m_MetallicMapEnabled ? material->m_MetallicMap->FilePath : "");
        out << YAML::EndMap;

        std::ofstream fout(path);
        fout << out.c_str();
    }

    Ref<Material> MaterialSerializer::Deserialize(const char* path)
    {
        std::ifstream in(path);
        std::stringstream ss;
        ss << in.rdbuf();

        YAML::Node data = YAML::Load(ss.str());

        Ref<Material> material = CreateRef<Material>();
        material->Handle = data["Handle"].as<uint64_t>();

        material->m_Name = data["Name"].as<std::string>();
        material->m_Albedo = data["Albedo"].as<glm::vec3>();
        material->m_Roughness = data["Roughness"].as<float>();
        material->m_Metallic = data["Metallic"].as<float>();

        material->m_AlbedoMapEnabled = data["TextureMapEnabled"].as<bool>();
        if (auto map = data["TextureMap"].as<std::string>(); !map.empty())
            material->m_AlbedoMap = AssetManager::TryGetOrLoadAsset<Texture2D>(map);

        material->m_NormalMapEnabled = data["NormalMapEnabled"].as<bool>();
        if (auto map = data["NormalMap"].as<std::string>(); !map.empty())
            material->m_NormalMap = AssetManager::TryGetOrLoadAsset<Texture2D>(map);

        material->m_RoughnessMapEnabled = data["RoughnessMapEnabled"].as<bool>();
        if (auto map = data["RoughnessMap"].as<std::string>(); !map.empty())
            material->m_RoughnessMap = AssetManager::TryGetOrLoadAsset<Texture2D>(map);

        material->m_AmbientOcclusionMapEnabled = data["AmbientOcclusionMapEnabled"].as<bool>();
        if (auto map = data["AmbientOcclusionMap"].as<std::string>(); !map.empty())
            material->m_AmbientOcclusionMap = AssetManager::TryGetOrLoadAsset<Texture2D>(map);

        material->m_MetallicMapEnabled = data["MetallicMapEnabled"].as<bool>();
        if (auto map = data["MetallicMap"].as<std::string>(); !map.empty())
            material->m_MetallicMap = AssetManager::TryGetOrLoadAsset<Texture2D>(map);

        material->Update();
        return material;
    }
}
