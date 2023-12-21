#include "MaterialAsset.h"

#include <fstream>
#include <glm/gtc/type_ptr.hpp>

#include "Core/YamlUtils.h"
#include "Asset/AssetManager.h"

namespace Flameberry {

    MaterialAsset::MaterialAsset(const std::string& name, const Ref<__Material>& material)
        : m_Name(name), m_MaterialRef(material) // TODO: This should be changed immediately
    {
        // This is here to remind the developer to update the GPU representation of the Material Struct when it is updated in the shader
        FBY_ASSERT(sizeof(MaterialStructGPURepresentation) == m_MaterialRef->GetUniformDataSize(), "The GPU Representation of Material has a size ({}) which is not the same as actual Uniform size ({})", sizeof(MaterialStructGPURepresentation), m_MaterialRef->GetUniformDataSize());
    }

    void MaterialAsset::SetAlbedoMap(const Ref<Texture2D>& map)
    {
        m_AlbedoMap = map;
        m_MaterialRef->Set("u_TextureMapSampler", m_AlbedoMap);
    }

    void MaterialAsset::SetNormalMap(const Ref<Texture2D>& map)
    {
        m_NormalMap = map;
        m_MaterialRef->Set("u_NormalMapSampler", m_NormalMap);
    }

    void MaterialAsset::SetRoughnessMap(const Ref<Texture2D>& map)
    {
        m_RoughnessMap = map;
        m_MaterialRef->Set("u_RoughnessMapSampler", m_RoughnessMap);
    }

    void MaterialAsset::SetAmbientOcclusionMap(const Ref<Texture2D>& map)
    {
        m_AmbientOcclusionMap = map;
        m_MaterialRef->Set("u_AmbientOcclusionMapSampler", m_AmbientOcclusionMap);
    }

    void MaterialAsset::SetMetallicMap(const Ref<Texture2D>& map)
    {
        m_MetallicMap = map;
        m_MaterialRef->Set("u_MetallicMapSampler", m_MetallicMap);
    }

    void MaterialAssetSerializer::Serialize(const Ref<MaterialAsset>& materialAsset, const char* path)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Handle" << YAML::Value << materialAsset->Handle;
        out << YAML::Key << "Name" << YAML::Value << materialAsset->m_Name;

        const float* albedo = materialAsset->m_MaterialRef->GetArray<float>("u_Albedo");
        out << YAML::Key << "Albedo" << YAML::Value << glm::vec3(albedo[0], albedo[1], albedo[2]);

        out << YAML::Key << "Roughness" << YAML::Value << materialAsset->m_MaterialRef->Get<float>("u_Roughness");
        out << YAML::Key << "Metallic" << YAML::Value << materialAsset->m_MaterialRef->Get<float>("u_Metallic");
        out << YAML::Key << "TextureMapEnabled" << YAML::Value << (bool)materialAsset->m_MaterialRef->Get<float>("u_AlbedoMapEnabled");
        out << YAML::Key << "TextureMap" << YAML::Value << (materialAsset->m_AlbedoMap ? materialAsset->m_AlbedoMap->FilePath : "");
        out << YAML::Key << "NormalMapEnabled" << YAML::Value << (bool)materialAsset->m_MaterialRef->Get<float>("u_NormalMapEnabled");
        out << YAML::Key << "NormalMap" << YAML::Value << (materialAsset->m_NormalMap ? materialAsset->m_NormalMap->FilePath : "");
        out << YAML::Key << "RoughnessMapEnabled" << YAML::Value << (bool)materialAsset->m_MaterialRef->Get<float>("u_RoughnessMapEnabled");
        out << YAML::Key << "RoughnessMap" << YAML::Value << (materialAsset->m_RoughnessMap ? materialAsset->m_RoughnessMap->FilePath : "");
        out << YAML::Key << "AmbientOcclusionMapEnabled" << YAML::Value << (bool)materialAsset->m_MaterialRef->Get<float>("u_AmbientOcclusionEnabled");
        out << YAML::Key << "AmbientOcclusionMap" << YAML::Value << (materialAsset->m_AmbientOcclusionMap ? materialAsset->m_AmbientOcclusionMap->FilePath : "");
        out << YAML::Key << "MetallicMapEnabled" << YAML::Value << (bool)materialAsset->m_MaterialRef->Get<float>("u_MetallicMapEnabled");
        out << YAML::Key << "MetallicMap" << YAML::Value << (materialAsset->m_MetallicMap ? materialAsset->m_MetallicMap->FilePath : "");
        out << YAML::EndMap;

        std::ofstream fout(path);
        fout << out.c_str();
    }

    Ref<MaterialAsset> MaterialAssetSerializer::Deserialize(const char* path)
    {
        std::ifstream in(path);
        std::stringstream ss;
        ss << in.rdbuf();

        YAML::Node data = YAML::Load(ss.str());

#if 1
        Ref<Shader> shader = CreateRef<Shader>(FBY_PROJECT_DIR"Flameberry/shaders/vulkan/bin/mesh.vert.spv", FBY_PROJECT_DIR"Flameberry/shaders/vulkan/bin/mesh.frag.spv");
        Ref<__Material> mat = CreateRef<__Material>(shader);
        Ref<MaterialAsset> materialAsset = CreateRef<MaterialAsset>(data["Name"].as<std::string>(), mat);
#endif

        materialAsset->Handle = data["Handle"].as<uint64_t>();

        materialAsset->SetAlbedo(data["Albedo"].as<glm::vec3>());
        materialAsset->SetRoughness(data["Roughness"].as<float>());
        materialAsset->SetMetallic(data["Metallic"].as<float>());
        materialAsset->SetAlbedoMapEnabled(data["TextureMapEnabled"].as<bool>());
        materialAsset->SetNormalMapEnabled(data["NormalMapEnabled"].as<bool>());
        materialAsset->SetRoughnessMapEnabled(data["RoughnessMapEnabled"].as<bool>());
        materialAsset->SetAmbientOcclusionMapEnabled(data["AmbientOcclusionMapEnabled"].as<bool>());
        materialAsset->SetMetallicMapEnabled(data["MetallicMapEnabled"].as<bool>());

        // TODO: Batch update the descriptor set of `m_MaterialRef`
        if (auto map = data["TextureMap"].as<std::string>(); !map.empty())
            materialAsset->SetAlbedoMap(AssetManager::TryGetOrLoadAsset<Texture2D>(map));

        if (auto map = data["NormalMap"].as<std::string>(); !map.empty())
            materialAsset->SetNormalMap(AssetManager::TryGetOrLoadAsset<Texture2D>(map));

        if (auto map = data["RoughnessMap"].as<std::string>(); !map.empty())
            materialAsset->SetRoughnessMap(AssetManager::TryGetOrLoadAsset<Texture2D>(map));

        if (auto map = data["AmbientOcclusionMap"].as<std::string>(); !map.empty())
            materialAsset->SetAmbientOcclusionMap(AssetManager::TryGetOrLoadAsset<Texture2D>(map));

        if (auto map = data["MetallicMap"].as<std::string>(); !map.empty())
            materialAsset->SetMetallicMap(AssetManager::TryGetOrLoadAsset<Texture2D>(map));

        return materialAsset;
    }

}
