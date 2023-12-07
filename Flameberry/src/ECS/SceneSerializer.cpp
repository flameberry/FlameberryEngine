#include "SceneSerializer.h"

#include <fstream>

#include "Core/YamlUtils.h"
#include "Core/Timer.h"
#include "Components.h"

#include "Renderer/VulkanContext.h"
#include "Asset/AssetManager.h"
#include "Asset/MeshLoader.h"

namespace Flameberry {
    static std::string ProjectionTypeEnumToString(ProjectionType type)
    {
        switch (type)
        {
            case ProjectionType::Orthographic: return "Orthographic";
            case ProjectionType::Perspective: return "Perspective";
        }
    }

    static ProjectionType ProjectionTypeStringToEnum(const std::string& type)
    {
        if (type == "Orthographic")
            return ProjectionType::Orthographic;
        else if (type == "Perspective")
            return ProjectionType::Perspective;
    }

    static std::string RigidBodyTypeEnumToString(RigidBodyComponent::RigidBodyType type)
    {
        switch (type)
        {
            case RigidBodyComponent::RigidBodyType::Static: return "Static";
            case RigidBodyComponent::RigidBodyType::Dynamic: return "Dynamic";
        }
    }

    static RigidBodyComponent::RigidBodyType RigidBodyTypeStringToEnum(const std::string& type)
    {
        if (type == "Static")
            return RigidBodyComponent::RigidBodyType::Static;
        else if (type == "Dynamic")
            return RigidBodyComponent::RigidBodyType::Dynamic;
    }

    static std::string AxisTypeEnumToString(AxisType type)
    {
        switch (type)
        {
            case AxisType::X: return "X";
            case AxisType::Y: return "Y";
            case AxisType::Z: return "Z";
        }
    }

    static AxisType AxisTypeStringToEnum(const std::string& type)
    {
        if (type == "X")
            return AxisType::X;
        else if (type == "Y")
            return AxisType::Y;
        else if (type == "Z")
            return AxisType::Z;
    }

    std::shared_ptr<Scene> SceneSerializer::DeserializeIntoNewScene(const char* path)
    {
        std::shared_ptr<Scene> newScene = std::make_shared<Scene>();
        if (DeserializeIntoExistingScene(path, newScene))
            return newScene;
        return nullptr;
    }

    bool SceneSerializer::DeserializeIntoExistingScene(const char* path, const std::shared_ptr<Scene>& destScene)
    {
        FBY_SCOPED_TIMER("Scene Deserialization");

        std::ifstream in(path);
        std::stringstream ss;
        ss << in.rdbuf();

        YAML::Node data = YAML::Load(ss.str());
        if (!data["Scene"])
        {
            FBY_ERROR("Failed to load scene [{}]: 'Scene' attribute not present in file!", path);
            return false;
        };

        destScene->m_Name = data["Scene"].as<std::string>();
        
        destScene->m_Registry->clear();

        auto entities = data["Entities"];
        if (entities)
        {
            for (auto entity : entities)
            {
                if (entity)
                {
                    // Deserialize entity
                    auto deserializedEntity = destScene->m_Registry->create();
                    if (auto id = entity["Entity"]; id)
                    {
                        auto& IDComp = destScene->m_Registry->emplace<IDComponent>(deserializedEntity);
                        IDComp.ID = id.as<uint64_t>();
                    }

                    if (auto tag = entity["TagComponent"]; tag)
                    {
                        auto& tagComp = destScene->m_Registry->emplace<TagComponent>(deserializedEntity);
                        tagComp.Tag = tag.as<std::string>();
                    }

                    if (auto transform = entity["TransformComponent"]; transform)
                    {
                        auto& transformComp = destScene->m_Registry->emplace<TransformComponent>(deserializedEntity);
                        transformComp.Translation = transform["Translation"].as<glm::vec3>();
                        transformComp.Rotation = transform["Rotation"].as<glm::vec3>();
                        transformComp.Scale = transform["Scale"].as<glm::vec3>();
                    }

                    if (auto camera = entity["CameraComponent"]; camera)
                    {
                        auto& cameraComp = destScene->m_Registry->emplace<CameraComponent>(deserializedEntity);
                        cameraComp.IsPrimary = camera["IsPrimary"].as<bool>();

                        ProjectionType type = ProjectionTypeStringToEnum(camera["ProjectionType"].as<std::string>());
                        float aspectRatio = camera["AspectRatio"].as<float>();
                        float FOV_or_Zoom = camera["FOV/Zoom"].as<float>();
                        float near = camera["Near"].as<float>();
                        float far = camera["Far"].as<float>();

                        switch (type)
                        {
                            case ProjectionType::Orthographic:
                                cameraComp.Camera.SetOrthographic(aspectRatio, FOV_or_Zoom, near, far);
                                break;
                            case ProjectionType::Perspective:
                                cameraComp.Camera.SetPerspective(aspectRatio, FOV_or_Zoom, near, far);
                                break;
                        }
                    }

                    if (auto mesh = entity["MeshComponent"]; mesh)
                    {
                        auto& meshComp = destScene->m_Registry->emplace<MeshComponent>(deserializedEntity, 0);
                        meshComp.MeshHandle = mesh["MeshHandle"].as<uint64_t>();

                        for (auto entry : mesh["OverridenMaterialTable"]) // TODO: Update with different format
                        {
                            auto mat = AssetManager::TryGetOrLoadAsset<Material>(entry["Material"].as<std::string>());
                            meshComp.OverridenMaterialTable[entry["SubmeshIndex"].as<uint32_t>()] = mat->Handle;
                        }
                    }
                    
                    if (auto skyLight = entity["SkyLightComponent"]; skyLight)
                    {
                        auto& skyLightComp = destScene->m_Registry->emplace<SkyLightComponent>(deserializedEntity);
                        skyLightComp.Color = skyLight["Color"].as<glm::vec3>();
                        skyLightComp.Intensity = skyLight["Intensity"].as<float>();
                        skyLightComp.EnableSkyMap = skyLight["EnableSkyMap"].as<bool>();
                        
                        std::string skymapPath = skyLight["SkyMap"].as<std::string>();
                        skyLightComp.SkyMap = ((skymapPath != "") ? AssetManager::TryGetOrLoadAsset<Texture2D>(skyLight["SkyMap"].as<std::string>())->Handle : AssetHandle(0));
                    }
                    
                    if (auto light = entity["DirectionalLightComponent"]; light)
                    {
                        auto& lightComp = destScene->m_Registry->emplace<DirectionalLightComponent>(deserializedEntity);
                        lightComp.Color = light["Color"].as<glm::vec3>();
                        lightComp.Intensity = light["Intensity"].as<float>();
                        lightComp.LightSize = light["LightSize"].as<float>();
                    }

                    if (auto light = entity["PointLightComponent"]; light)
                    {
                        auto& lightComp = destScene->m_Registry->emplace<PointLightComponent>(deserializedEntity);
                        lightComp.Color = light["Color"].as<glm::vec3>();
                        lightComp.Intensity = light["Intensity"].as<float>();
                    }

                    if (auto rigidBody = entity["RigidBodyComponent"]; rigidBody)
                    {
                        auto& rbComp = destScene->m_Registry->emplace<RigidBodyComponent>(deserializedEntity);
                        rbComp.Type = RigidBodyTypeStringToEnum(rigidBody["Type"].as<std::string>());
                        rbComp.Density = rigidBody["Density"].as<float>();
                        rbComp.StaticFriction = rigidBody["StaticFriction"].as<float>();
                        rbComp.DynamicFriction = rigidBody["DynamicFriction"].as<float>();
                        rbComp.Restitution = rigidBody["Restitution"].as<float>();
                    }

                    if (auto boxCollider = entity["BoxColliderComponent"]; boxCollider)
                    {
                        auto& bcComp = destScene->m_Registry->emplace<BoxColliderComponent>(deserializedEntity);
                        bcComp.Size = boxCollider["Size"].as<glm::vec3>();
                    }

                    if (auto sphereCollider = entity["SphereColliderComponent"]; sphereCollider)
                    {
                        auto& scComp = destScene->m_Registry->emplace<SphereColliderComponent>(deserializedEntity);
                        scComp.Radius = sphereCollider["Radius"].as<float>();
                    }

                    if (auto capsuleCollider = entity["CapsuleColliderComponent"]; capsuleCollider)
                    {
                        auto& ccComp = destScene->m_Registry->emplace<CapsuleColliderComponent>(deserializedEntity);
                        ccComp.Axis = AxisTypeStringToEnum(capsuleCollider["Axis"].as<std::string>());
                        ccComp.Radius = capsuleCollider["Radius"].as<float>();
                        ccComp.Height = capsuleCollider["Height"].as<float>();
                    }
                }
            }
        }

        auto meshes = data["Meshes"];
        if (meshes)
        {
            for (auto mesh : meshes)
            {
                auto meshAsset = MeshLoader::LoadMesh(mesh["FilePath"].as<std::string>().c_str());
                meshAsset->Handle = mesh["Mesh"].as<uint64_t>();
                AssetManager::RegisterAsset(meshAsset);
            }
        }
        return true;
    }

    void SceneSerializer::SerializeSceneToFile(const char* path, const std::shared_ptr<Scene>& srcScene)
    {
        FBY_SCOPED_TIMER("Serialization");

        std::string scenePath(path);
        uint32_t lastSlashPosition = scenePath.find_last_of('/') + 1;
        uint32_t lastDotPosition = scenePath.find_last_of('.');
        std::string sceneName = scenePath.substr(lastSlashPosition, lastDotPosition - lastSlashPosition);

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << sceneName;

        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        std::set<UUID> meshHandles;

        srcScene->m_Registry->for_each([&](fbentt::entity entity)
            {
                SerializeEntity(out, entity, srcScene, meshHandles);
            }
        );
        out << YAML::EndSeq; // Entities

        // Serialize Meshes loaded
        out << YAML::Key << "Meshes" << YAML::Value << YAML::BeginSeq;

        for (const auto& uuid : meshHandles)
        {
            auto mesh = AssetManager::GetAsset<StaticMesh>(uuid);

            out << YAML::BeginMap;
            out << YAML::Key << "Mesh" << YAML::Value << mesh->Handle;
            out << YAML::Key << "FilePath" << YAML::Value << mesh->FilePath;
            out << YAML::EndMap; // Mesh
        }
        out << YAML::EndSeq; // Meshes
        out << YAML::EndMap; // Scene

        std::ofstream fout(scenePath);
        fout << out.c_str();
    }

    void SceneSerializer::SerializeEntity(YAML::Emitter& out, const fbentt::entity& entity, const std::shared_ptr<Scene>& scene, std::set<UUID>& meshUUIDs)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "Entity" << YAML::Value << scene->m_Registry->get<IDComponent>(entity).ID;

        out << YAML::Key << "TagComponent" << YAML::Value << scene->m_Registry->get<TagComponent>(entity).Tag;

        if (scene->m_Registry->has<TransformComponent>(entity))
        {
            auto& transform = scene->m_Registry->get<TransformComponent>(entity);
            out << YAML::Key << "TransformComponent" << YAML::BeginMap;
            out << YAML::Key << "Translation" << YAML::Value << transform.Translation;
            out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
            out << YAML::Key << "Scale" << YAML::Value << transform.Scale;
            out << YAML::EndMap; // Transform Component
        }

        if (scene->m_Registry->has<CameraComponent>(entity))
        {
            auto& cameraComp = scene->m_Registry->get<CameraComponent>(entity);
            const auto& settings = cameraComp.Camera.GetSettings();
            out << YAML::Key << "CameraComponent" << YAML::BeginMap;
            out << YAML::Key << "IsPrimary" << YAML::Value << cameraComp.IsPrimary;
            out << YAML::Key << "ProjectionType" << YAML::Value << ProjectionTypeEnumToString(settings.ProjectionType);
            out << YAML::Key << "AspectRatio" << YAML::Value << settings.AspectRatio;
            out << YAML::Key << "FOV/Zoom" << YAML::Value << settings.FOV;
            out << YAML::Key << "Near" << YAML::Value << settings.Near;
            out << YAML::Key << "Far" << YAML::Value << settings.Far;
            out << YAML::EndMap; // Camera Component
        }

        if (scene->m_Registry->has<MeshComponent>(entity))
        {
            auto& mesh = scene->m_Registry->get<MeshComponent>(entity);
            out << YAML::Key << "MeshComponent" << YAML::BeginMap;
            out << YAML::Key << "MeshHandle" << YAML::Value << (uint64_t)(mesh.MeshHandle);

            out << YAML::Key << "OverridenMaterialTable" << YAML::BeginSeq;
            for (const auto& [submeshIndex, materialHandle] : mesh.OverridenMaterialTable)
            {
                out << YAML::BeginMap;
                out << YAML::Key << "SubmeshIndex" << YAML::Value << submeshIndex;

                auto mat = AssetManager::GetAsset<Material>(materialHandle);
                out << YAML::Key << "Material" << YAML::Value << mat->FilePath;
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
            out << YAML::EndMap; // Mesh Component

            meshUUIDs.insert(mesh.MeshHandle);
        }
        
        if (scene->m_Registry->has<SkyLightComponent>(entity))
        {
            auto& skyLight = scene->m_Registry->get<SkyLightComponent>(entity);
            out << YAML::Key << "SkyLightComponent" << YAML::BeginMap;
            out << YAML::Key << "Color" << YAML::Value << skyLight.Color;
            out << YAML::Key << "Intensity" << YAML::Value << skyLight.Intensity;
            out << YAML::Key << "EnableSkyMap" << YAML::Value << skyLight.EnableSkyMap;
            out << YAML::Key << "SkyMap" << YAML::Value << (AssetManager::IsAssetHandleValid(skyLight.SkyMap) ? AssetManager::GetAsset<Texture2D>(skyLight.SkyMap)->FilePath : "");
            out << YAML::EndMap; // Sky Light Component
        }
        
        if (scene->m_Registry->has<DirectionalLightComponent>(entity))
        {
            auto& light = scene->m_Registry->get<DirectionalLightComponent>(entity);
            out << YAML::Key << "DirectionalLightComponent" << YAML::BeginMap;
            out << YAML::Key << "Color" << YAML::Value << light.Color;
            out << YAML::Key << "Intensity" << YAML::Value << light.Intensity;
            out << YAML::Key << "LightSize" << YAML::Value << light.LightSize;
            out << YAML::EndMap; // Directional Light Component
        }

        if (scene->m_Registry->has<PointLightComponent>(entity))
        {
            auto& light = scene->m_Registry->get<PointLightComponent>(entity);
            out << YAML::Key << "PointLightComponent" << YAML::BeginMap;
            out << YAML::Key << "Color" << YAML::Value << light.Color;
            out << YAML::Key << "Intensity" << YAML::Value << light.Intensity;
            out << YAML::EndMap; // Point Light Component
        }

        if (scene->m_Registry->has<RigidBodyComponent>(entity))
        {
            auto& rigidBody = scene->m_Registry->get<RigidBodyComponent>(entity);
            out << YAML::Key << "RigidBodyComponent" << YAML::BeginMap;
            out << YAML::Key << "Type" << YAML::Value << RigidBodyTypeEnumToString(rigidBody.Type);
            out << YAML::Key << "Density" << YAML::Value << rigidBody.Density;
            out << YAML::Key << "StaticFriction" << YAML::Value << rigidBody.StaticFriction;
            out << YAML::Key << "DynamicFriction" << YAML::Value << rigidBody.DynamicFriction;
            out << YAML::Key << "Restitution" << YAML::Value << rigidBody.Restitution;
            out << YAML::Key << YAML::EndMap; // Rigid Body Component
        }

        if (scene->m_Registry->has<BoxColliderComponent>(entity))
        {
            auto& boxCollider = scene->m_Registry->get<BoxColliderComponent>(entity);
            out << YAML::Key << "BoxColliderComponent" << YAML::BeginMap;
            out << YAML::Key << "Size" << YAML::Value << boxCollider.Size;
            out << YAML::Key << YAML::EndMap; // Box Collider Component
        }

        if (scene->m_Registry->has<SphereColliderComponent>(entity))
        {
            auto& sphereCollider = scene->m_Registry->get<SphereColliderComponent>(entity);
            out << YAML::Key << "SphereColliderComponent" << YAML::BeginMap;
            out << YAML::Key << "Radius" << YAML::Value << sphereCollider.Radius;
            out << YAML::Key << YAML::EndMap; // Sphere Collider Component
        }

        if (scene->m_Registry->has<CapsuleColliderComponent>(entity))
        {
            auto& capsuleCollider = scene->m_Registry->get<CapsuleColliderComponent>(entity);
            out << YAML::Key << "CapsuleColliderComponent" << YAML::BeginMap;
            out << YAML::Key << "Axis" << YAML::Value << AxisTypeEnumToString(capsuleCollider.Axis);
            out << YAML::Key << "Radius" << YAML::Value << capsuleCollider.Radius;
            out << YAML::Key << "Height" << YAML::Value << capsuleCollider.Height;
            out << YAML::Key << YAML::EndMap; // Capsule Collider Component
        }

        out << YAML::EndMap; // Entity
    }
}
