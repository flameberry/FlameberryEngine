#include "Material.h"

#include <fstream>
#include "Core/YamlUtils.h"

#include "Asset/AssetManager.h"

namespace Flameberry {
    void MaterialSerializer::Serialize(const std::shared_ptr<Material>& material, const char* path)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Handle" << YAML::Value << material->Handle;
        out << YAML::Key << "Name" << YAML::Value << material->Name;
        out << YAML::Key << "Albedo" << YAML::Value << material->Albedo;
        out << YAML::Key << "Roughness" << YAML::Value << material->Roughness;
        out << YAML::Key << "Metallic" << YAML::Value << material->Metallic;
        out << YAML::Key << "TextureMapEnabled" << YAML::Value << material->TextureMapEnabled;
        out << YAML::Key << "TextureMap" << YAML::Value << (material->TextureMapEnabled ? material->TextureMap->FilePath : "");
        out << YAML::Key << "NormalMapEnabled" << YAML::Value << material->NormalMapEnabled;
        out << YAML::Key << "NormalMap" << YAML::Value << (material->NormalMapEnabled ? material->NormalMap->FilePath : "");
        out << YAML::Key << "RoughnessMapEnabled" << YAML::Value << material->RoughnessMapEnabled;
        out << YAML::Key << "RoughnessMap" << YAML::Value << (material->RoughnessMapEnabled ? material->RoughnessMap->FilePath : "");
        out << YAML::Key << "AmbientOcclusionMapEnabled" << YAML::Value << material->AmbientOcclusionMapEnabled;
        out << YAML::Key << "AmbientOcclusionMap" << YAML::Value << (material->AmbientOcclusionMapEnabled ? material->AmbientOcclusionMap->FilePath : "");
        out << YAML::Key << "MetallicMapEnabled" << YAML::Value << material->MetallicMapEnabled;
        out << YAML::Key << "MetallicMap" << YAML::Value << (material->MetallicMapEnabled ? material->MetallicMap->FilePath : "");
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

        std::shared_ptr<Material> material = std::make_shared<Material>();
        material->Handle = data["Handle"].as<uint64_t>();

        material->Name = data["Name"].as<std::string>();
        material->Albedo = data["Albedo"].as<glm::vec3>();
        material->Roughness = data["Roughness"].as<float>();
        material->Metallic = data["Metallic"].as<float>();

        material->TextureMapEnabled = data["TextureMapEnabled"].as<bool>();
        if (auto map = data["TextureMap"].as<std::string>(); !map.empty())
            material->TextureMap = AssetManager::TryGetOrLoadAsset<Texture2D>(map);

        material->NormalMapEnabled = data["NormalMapEnabled"].as<bool>();
        if (auto map = data["NormalMap"].as<std::string>(); !map.empty())
            material->NormalMap = AssetManager::TryGetOrLoadAsset<Texture2D>(map);

        material->RoughnessMapEnabled = data["RoughnessMapEnabled"].as<bool>();
        if (auto map = data["RoughnessMap"].as<std::string>(); !map.empty())
            material->RoughnessMap = AssetManager::TryGetOrLoadAsset<Texture2D>(map);

        material->AmbientOcclusionMapEnabled = data["AmbientOcclusionMapEnabled"].as<bool>();
        if (auto map = data["AmbientOcclusionMap"].as<std::string>(); !map.empty())
            material->AmbientOcclusionMap = AssetManager::TryGetOrLoadAsset<Texture2D>(map);

        material->MetallicMapEnabled = data["MetallicMapEnabled"].as<bool>();
        if (auto map = data["MetallicMap"].as<std::string>(); !map.empty())
            material->MetallicMap = AssetManager::TryGetOrLoadAsset<Texture2D>(map);

        return material;
    }
}
