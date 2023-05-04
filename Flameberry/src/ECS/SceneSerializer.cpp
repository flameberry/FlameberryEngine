#include "SceneSerializer.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "Core/Timer.h"
#include "Component.h"

#include "Renderer/Vulkan/VulkanContext.h"

namespace YAML {
    template<>
    struct convert<glm::vec3>
    {
        static Node encode(const glm::vec3& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, glm::vec3& rhs)
        {
            if (!node.IsSequence() || node.size() != 3)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };

    template<>
    struct convert<glm::vec4>
    {
        static Node encode(const glm::vec4& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.push_back(rhs.w);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, glm::vec4& rhs)
        {
            if (!node.IsSequence() || node.size() != 4)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            rhs.w = node[3].as<float>();
            return true;
        }
    };
}

namespace Flameberry {
    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        return out;
    }

    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
        return out;
    }

    SceneSerializer::SceneSerializer(const std::shared_ptr<Scene>& scene)
        : m_ActiveScene(scene)
    {
    }

    SceneSerializer::~SceneSerializer()
    {
    }

    bool SceneSerializer::SerializeScene(const std::string& scenePath)
    {
        FL_SCOPED_TIMER("Serialization");

        uint32_t lastSlashPosition = scenePath.find_last_of('/') + 1;
        uint32_t lastDotPosition = scenePath.find_last_of('.');
        std::string sceneName = scenePath.substr(lastSlashPosition, lastDotPosition - lastSlashPosition);

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << sceneName;

        // Environment Map
        out << YAML::Key << "EnvironmentMap" << YAML::Value << YAML::BeginMap;

        auto& env = m_ActiveScene->m_SceneData.ActiveEnvironmentMap;
        out << YAML::Key << "ClearColor" << YAML::Value << env.ClearColor;
        // out << YAML::Key << "Skybox" << YAML::Value << env.ActiveSkybox->GetFolderPath();
        out << YAML::Key << "EnableSkybox" << YAML::Value << env.EnableSkybox;
        out << YAML::Key << "Reflections" << YAML::Value << env.Reflections;

        out << YAML::Key << "DirectionalLight" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Direction" << YAML::Value << env.DirLight.Direction;
        out << YAML::Key << "Color" << YAML::Value << env.DirLight.Color;
        out << YAML::Key << "Intensity" << YAML::Value << env.DirLight.Intensity;
        out << YAML::EndMap; // Directional Light

        out << YAML::EndMap; // EnvironmentMap

        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        m_ActiveScene->m_Registry->each([&](ecs::entity_handle& entity)
            {
                SerializeEntity(out, entity);
            }
        );
        out << YAML::EndSeq; // Entities

        // Serialize Meshes loaded
        out << YAML::Key << "Meshes" << YAML::Value << YAML::BeginSeq;

        for (const auto& mesh : m_ActiveScene->m_SceneData.Meshes)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "Mesh" << YAML::Value << mesh->GetName();
            out << YAML::Key << "FilePath" << YAML::Value << mesh->GetFilePath();
            out << YAML::EndMap; // Mesh
        }

        out << YAML::EndSeq; // Meshes

        // Serialize Materials
        out << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq;

        for (const auto& [materialName, material] : m_ActiveScene->m_SceneData.Materials)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "Material" << YAML::Value << materialName;
            out << YAML::Key << "Albedo" << YAML::Value << material.Albedo;
            out << YAML::Key << "Roughness" << YAML::Value << material.Roughness;
            out << YAML::Key << "Metallic" << YAML::Value << material.Metallic;
            out << YAML::Key << "TextureMapEnabled" << YAML::Value << material.TextureMapEnabled;
            if (material.TextureMapEnabled)
                out << YAML::Key << "TextureMap" << YAML::Value << material.TextureMap->GetFilePath();
            else
                out << YAML::Key << "TextureMap" << YAML::Value << "None";
            out << YAML::EndMap; // Material
        }

        out << YAML::EndSeq; // Materials
        out << YAML::EndMap; // Scene

        std::ofstream fout(scenePath);
        fout << out.c_str();

        return true;
    }

    void SceneSerializer::SerializeEntity(YAML::Emitter& out, ecs::entity_handle& entity)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "Entity" << YAML::Value << m_ActiveScene->m_Registry->get<IDComponent>(entity).ID;

        out << YAML::Key << "TagComponent" << YAML::Value << m_ActiveScene->m_Registry->get<TagComponent>(entity).Tag;

        if (m_ActiveScene->m_Registry->has<TransformComponent>(entity))
        {
            auto& transform = m_ActiveScene->m_Registry->get<TransformComponent>(entity);
            out << YAML::Key << "TransformComponent" << YAML::BeginMap;
            out << YAML::Key << "Translation" << YAML::Value << transform.translation;
            out << YAML::Key << "Rotation" << YAML::Value << transform.rotation;
            out << YAML::Key << "Scale" << YAML::Value << transform.scale;
            out << YAML::EndMap; // Transform Component
        }

        if (m_ActiveScene->m_Registry->has<SpriteRendererComponent>(entity))
        {
            auto& sprite = m_ActiveScene->m_Registry->get<SpriteRendererComponent>(entity);
            out << YAML::Key << "SpriteRendererComponent" << YAML::BeginMap;
            out << YAML::Key << "Color" << YAML::Value << sprite.Color;
            out << YAML::Key << "TextureFilePath" << YAML::Value << sprite.TextureFilePath;
            out << YAML::EndMap; // Sprite Renderer Component
        }

        if (m_ActiveScene->m_Registry->has<MeshComponent>(entity))
        {
            auto& mesh = m_ActiveScene->m_Registry->get<MeshComponent>(entity);
            out << YAML::Key << "MeshComponent" << YAML::BeginMap;
            out << YAML::Key << "MeshIndex" << YAML::Value << mesh.MeshIndex;
            out << YAML::Key << "MaterialName" << YAML::Value << mesh.MaterialName;
            out << YAML::EndMap; // Mesh Component
        }

        if (m_ActiveScene->m_Registry->has<LightComponent>(entity))
        {
            auto& light = m_ActiveScene->m_Registry->get<LightComponent>(entity);
            out << YAML::Key << "LightComponent" << YAML::BeginMap;
            out << YAML::Key << "Color" << YAML::Value << light.Color;
            out << YAML::Key << "Intensity" << YAML::Value << light.Intensity;
            out << YAML::EndMap; // Light Component
        }

        out << YAML::EndMap; // Entity
    }

    bool SceneSerializer::DeserializeScene(const char* scenePath)
    {
        FL_SCOPED_TIMER("Deserialization");

        std::ifstream in(scenePath);
        std::stringstream ss;
        ss << in.rdbuf();

        YAML::Node data = YAML::Load(ss.str());
        if (!data["Scene"])
        {
            FL_ERROR("Failed to load scene [{0}]: 'Scene' attribute not present in file!", scenePath);
            return false;
        };

        m_ActiveScene->m_SceneData.Name = data["Scene"].as<std::string>();

        // Clearing existing scene // TODO: Remove Active Environment Map
        m_ActiveScene->m_Registry->clear();
        m_ActiveScene->m_SceneData.Materials.clear();
        m_ActiveScene->m_SceneData.Meshes.clear();

        // Deserialize Environment Map
        auto& env = m_ActiveScene->m_SceneData.ActiveEnvironmentMap;

        auto envMapNode = data["EnvironmentMap"];
        env.ClearColor = envMapNode["ClearColor"].as<glm::vec3>();
        // env.ActiveSkybox = std::make_shared<Skybox>(envMapNode["Skybox"].as<std::string>().c_str());
        env.EnableSkybox = envMapNode["EnableSkybox"].as<bool>();
        env.Reflections = envMapNode["Reflections"].as<bool>();
        env.DirLight.Direction = envMapNode["DirectionalLight"]["Direction"].as<glm::vec3>();
        env.DirLight.Color = envMapNode["DirectionalLight"]["Color"].as<glm::vec3>();
        env.DirLight.Intensity = envMapNode["DirectionalLight"]["Intensity"].as<float>();

        auto entities = data["Entities"];
        if (entities)
        {
            for (auto entity : entities)
            {
                if (entity)
                {
                    // Deserialize entity
                    auto deserializedEntity = m_ActiveScene->m_Registry->create();
                    if (auto id = entity["Entity"]; id)
                    {
                        auto& IDComp = m_ActiveScene->m_Registry->emplace<IDComponent>(deserializedEntity);
                        IDComp.ID = id.as<uint64_t>();
                    }

                    if (auto tag = entity["TagComponent"]; tag)
                    {
                        auto& tagComp = m_ActiveScene->m_Registry->emplace<TagComponent>(deserializedEntity);
                        tagComp.Tag = tag.as<std::string>();
                    }

                    if (auto transform = entity["TransformComponent"]; transform) // Deserialize entity
                    {
                        auto& transformComp = m_ActiveScene->m_Registry->emplace<TransformComponent>(deserializedEntity);
                        transformComp.translation = transform["Translation"].as<glm::vec3>();
                        transformComp.rotation = transform["Rotation"].as<glm::vec3>();
                        transformComp.scale = transform["Scale"].as<glm::vec3>();
                    }

                    if (auto sprite = entity["SpriteRendererComponent"]; sprite) // Deserialize entity
                    {
                        auto& spriteComp = m_ActiveScene->m_Registry->emplace<SpriteRendererComponent>(deserializedEntity);
                        spriteComp.Color = sprite["Color"].as<glm::vec4>();
                        spriteComp.TextureFilePath = sprite["TextureFilePath"].as<std::string>();
                    }

                    if (auto mesh = entity["MeshComponent"]; mesh)
                    {
                        auto& meshComp = m_ActiveScene->m_Registry->emplace<MeshComponent>(deserializedEntity);
                        meshComp.MeshIndex = mesh["MeshIndex"].as<uint32_t>();
                        meshComp.MaterialName = mesh["MaterialName"].as<std::string>();
                    }

                    if (auto light = entity["LightComponent"]; light)
                    {
                        auto& lightComp = m_ActiveScene->m_Registry->emplace<LightComponent>(deserializedEntity);
                        lightComp.Color = light["Color"].as<glm::vec3>();
                        lightComp.Intensity = light["Intensity"].as<float>();
                    }
                }
            }
        }

        auto meshes = data["Meshes"];
        if (meshes)
        {
            for (auto mesh : meshes)
            {
                if (mesh)
                    m_ActiveScene->m_SceneData.Meshes.emplace_back(VulkanMesh::TryGetOrLoadMesh(mesh["FilePath"].as<std::string>()));
            }
        }

        auto materials = data["Materials"];
        if (materials)
        {
            for (auto material : materials)
            {
                if (material)
                {
                    if (auto materialName = material["Material"]; materialName)
                    {
                        bool textureMapEnabled = material["TextureMapEnabled"].as<bool>();
                        m_ActiveScene->m_SceneData.Materials[materialName.as<std::string>()] = Material{
                            material["Albedo"].as<glm::vec3>(),
                            material["Roughness"].as<float>(),
                            material["Metallic"].as<bool>(),
                            textureMapEnabled,
                            textureMapEnabled ? VulkanTexture::TryGetOrLoadTexture(material["TextureMap"].as<std::string>().c_str()) : nullptr
                        };
                    }
                }
            }
        }
        return true;
    }
}
