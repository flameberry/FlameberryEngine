#include "Material.h"

#include <fstream>
#include "Core/YamlUtils.h"

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
        out << YAML::Key << "TextureMap" << YAML::Value << material->TextureMap->GetFilePath();
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
        material->Metallic = data["Metallic"].as<bool>();
        material->TextureMapEnabled = data["TextureMapEnabled"].as<bool>();
        if (material->TextureMapEnabled)
            material->TextureMap = VulkanTexture::TryGetOrLoadTexture(data["TextureMap"].as<std::string>());

        return material;
    }
}