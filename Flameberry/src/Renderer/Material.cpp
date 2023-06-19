#include "Material.h"

#include <fstream>
#include "Core/YamlUtils.h"

#include "AssetManager/AssetManager.h"

namespace Flameberry {
    std::shared_ptr<Material> Material::LoadFromFile(const char* path)
    {
        return MaterialSerializer::Deserialize(path);
    }

    Material::Material(UUID uuid)
        : m_UUID(uuid)
    {
    }

    void MaterialSerializer::Serialize(const std::shared_ptr<Material>& material, const char* path)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "UUID" << YAML::Value << material->GetUUID();
        out << YAML::Key << "Name" << YAML::Value << material->Name;
        out << YAML::Key << "Albedo" << YAML::Value << material->Albedo;
        out << YAML::Key << "Roughness" << YAML::Value << material->Roughness;
        out << YAML::Key << "Metallic" << YAML::Value << material->Metallic;
        out << YAML::Key << "TextureMapEnabled" << YAML::Value << material->TextureMapEnabled;
        out << YAML::Key << "TextureMap" << YAML::Value << (material->TextureMapEnabled ? material->TextureMap->GetFilePath() : "");
        out << YAML::Key << "NormalMapEnabled" << YAML::Value << material->NormalMapEnabled;
        out << YAML::Key << "NormalMap" << YAML::Value << (material->NormalMapEnabled ? material->NormalMap->GetFilePath() : "");
        out << YAML::Key << "RoughnessMapEnabled" << YAML::Value << material->RoughnessMapEnabled;
        out << YAML::Key << "RoughnessMap" << YAML::Value << (material->RoughnessMapEnabled ? material->RoughnessMap->GetFilePath() : "");
        out << YAML::Key << "AmbientOcclusionMapEnabled" << YAML::Value << material->AmbientOcclusionMapEnabled;
        out << YAML::Key << "AmbientOcclusionMap" << YAML::Value << (material->AmbientOcclusionMapEnabled ? material->AmbientOcclusionMap->GetFilePath() : "");
        out << YAML::Key << "MetallicMapEnabled" << YAML::Value << material->MetallicMapEnabled;
        out << YAML::Key << "MetallicMap" << YAML::Value << (material->MetallicMapEnabled ? material->MetallicMap->GetFilePath() : "");
        out << YAML::EndMap;

        std::ofstream fout(path);
        fout << out.c_str();
    }

    std::shared_ptr<Material> MaterialSerializer::Deserialize(const char* path)
    {
        std::ifstream in(path);
        std::stringstream ss;
        ss << in.rdbuf();

        YAML::Node data = YAML::Load(ss.str());

        std::shared_ptr<Material> material = std::make_shared<Material>(data["UUID"].as<uint64_t>());
        material->IsDerived = false;
        material->FilePath = path;
        material->Name = data["Name"].as<std::string>();
        material->Albedo = data["Albedo"].as<glm::vec3>();
        material->Roughness = data["Roughness"].as<float>();
        material->Metallic = data["Metallic"].as<float>();

        material->TextureMapEnabled = data["TextureMapEnabled"].as<bool>();
        if (auto map = data["TextureMap"].as<std::string>(); !map.empty())
            material->TextureMap = AssetManager::TryGetOrLoadAssetFromFile<Texture2D>(map);

        material->NormalMapEnabled = data["NormalMapEnabled"].as<bool>();
        if (auto map = data["NormalMap"].as<std::string>(); !map.empty())
            material->NormalMap = AssetManager::TryGetOrLoadAssetFromFile<Texture2D>(map);

        material->RoughnessMapEnabled = data["RoughnessMapEnabled"].as<bool>();
        if (auto map = data["RoughnessMap"].as<std::string>(); !map.empty())
            material->RoughnessMap = AssetManager::TryGetOrLoadAssetFromFile<Texture2D>(map);

        material->AmbientOcclusionMapEnabled = data["AmbientOcclusionMapEnabled"].as<bool>();
        if (auto map = data["AmbientOcclusionMap"].as<std::string>(); !map.empty())
            material->AmbientOcclusionMap = AssetManager::TryGetOrLoadAssetFromFile<Texture2D>(map);

        material->MetallicMapEnabled = data["MetallicMapEnabled"].as<bool>();
        if (auto map = data["MetallicMap"].as<std::string>(); !map.empty())
            material->MetallicMap = AssetManager::TryGetOrLoadAssetFromFile<Texture2D>(map);

        return material;
    }
}
