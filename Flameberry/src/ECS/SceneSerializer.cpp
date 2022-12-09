#include "SceneSerializer.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

#include "Core/Timer.h"

namespace Flameberry {
    SceneSerializer::SceneSerializer(const std::shared_ptr<Scene>& scene)
        : m_ActiveScene(scene)
    {
    }

    SceneSerializer::~SceneSerializer()
    {
    }

    bool SceneSerializer::SerializeScene(const char* scenePath)
    {
        FL_SCOPED_TIMER("Serialization");
        rapidjson::Document document;
        rapidjson::MemoryPoolAllocator allocator = document.GetAllocator();

        rapidjson::Value value(rapidjson::kStringType);
        rapidjson::Value entityJSONObject(rapidjson::kObjectType);
        rapidjson::Value materialJSONObject(rapidjson::kObjectType);

        value.SetString(m_ActiveScene->m_SceneData.Name.c_str(), m_ActiveScene->m_SceneData.Name.length());

        document.SetObject();
        document.AddMember("Scene", value, allocator);

        // Start with entities
        value.SetArray();
        document.AddMember("Entities", value, allocator);

        m_ActiveScene->m_Registry->each([&](entity_handle& entity) {
            entityJSONObject.SetObject();
        entityJSONObject.AddMember("UUID", m_ActiveScene->m_Registry->GetComponent<IDComponent>(entity)->ID, allocator);

        // Tag Component
        auto& tag = m_ActiveScene->m_Registry->GetComponent<TagComponent>(entity)->Tag;
        value.SetString(tag.c_str(), tag.length());
        entityJSONObject.AddMember("TagComponent", value, allocator);

        // Transform Component
        auto transformComponent = m_ActiveScene->m_Registry->GetComponent<TransformComponent>(entity);
        value.SetArray();
        value.PushBack(transformComponent->translation.x, allocator);
        value.PushBack(transformComponent->translation.y, allocator);
        value.PushBack(transformComponent->translation.z, allocator);
        value.PushBack(transformComponent->rotation.x, allocator);
        value.PushBack(transformComponent->rotation.y, allocator);
        value.PushBack(transformComponent->rotation.z, allocator);
        value.PushBack(transformComponent->scale.x, allocator);
        value.PushBack(transformComponent->scale.y, allocator);
        value.PushBack(transformComponent->scale.z, allocator);
        entityJSONObject.AddMember("TransformComponent", value, allocator);

        // Mesh Component
        if (m_ActiveScene->m_Registry->HasComponent<MeshComponent>(entity))
        {
            auto meshComponent = m_ActiveScene->m_Registry->GetComponent<MeshComponent>(entity);
            value.SetObject();
            entityJSONObject.AddMember("MeshComponent", value, allocator);
            entityJSONObject["MeshComponent"].AddMember("MeshIndex", meshComponent->MeshIndex, allocator);

            value.SetString(meshComponent->MaterialName.c_str(), meshComponent->MaterialName.length());
            entityJSONObject["MeshComponent"].AddMember("MaterialName", value, allocator);
        }
        document["Entities"].PushBack(entityJSONObject, allocator);
            });

        value.SetArray();
        document.AddMember("Materials", value, allocator);

        for (const auto& [name, material] : m_ActiveScene->m_SceneData.Materials)
        {
            materialJSONObject.SetObject();

            value.SetString(name.c_str(), name.length());
            materialJSONObject.AddMember("Name", value, allocator);

            value.SetArray();
            value.PushBack(material.Albedo.x, allocator);
            value.PushBack(material.Albedo.y, allocator);
            value.PushBack(material.Albedo.z, allocator);
            materialJSONObject.AddMember("Albedo", value, allocator);

            materialJSONObject.AddMember("Roughness", material.Roughness, allocator);
            materialJSONObject.AddMember("IsMetal", material.IsMetal, allocator);

            document["Materials"].PushBack(materialJSONObject, allocator);
        }

        FILE* fp = fopen(scenePath, "w");

        char writeBuffer[65536];
        rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));

        rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
        document.Accept(writer);
        fclose(fp);
        return true;
    }

    bool SceneSerializer::DeserializeScene(const char* scenePath)
    {
        FL_SCOPED_TIMER("Deserialization");
        FILE* fp = fopen(scenePath, "r");
        FL_ASSERT(fp, "Failed to open the scene: {0}", scenePath);

        char readBuffer[65536];
        rapidjson::FileReadStream istream(fp, readBuffer, sizeof(readBuffer));

        rapidjson::Document document;
        document.ParseStream(istream);
        FL_ASSERT(document.IsObject(), "Invalid JSON file!");

        m_ActiveScene->m_Registry->Clear();

        m_ActiveScene->m_SceneData.Materials.clear();
        m_ActiveScene->m_SceneData.Name = document["Scene"].GetString();

        const rapidjson::Value& entities = document["Entities"];
        for (const auto& entityDetails : entities.GetArray())
        {
            auto& entity = m_ActiveScene->m_Registry->CreateEntity();

            auto& id = entityDetails["UUID"];
            m_ActiveScene->m_Registry->AddComponent<IDComponent>(entity, id.GetUint64());

            auto& tagComponent = entityDetails["TagComponent"];
            m_ActiveScene->m_Registry->AddComponent<TagComponent>(entity)->Tag = tagComponent.IsString() ? entityDetails["TagComponent"].GetString() : "Empty";

            auto& transformComponent = entityDetails["TransformComponent"];
            if (transformComponent.IsArray())
            {
                auto transform = m_ActiveScene->m_Registry->AddComponent<TransformComponent>(entity);

                transform->translation.x = transformComponent.GetArray()[0].GetFloat();
                transform->translation.y = transformComponent.GetArray()[1].GetFloat();
                transform->translation.z = transformComponent.GetArray()[2].GetFloat();
                transform->rotation.x = transformComponent.GetArray()[3].GetFloat();
                transform->rotation.y = transformComponent.GetArray()[4].GetFloat();
                transform->rotation.z = transformComponent.GetArray()[5].GetFloat();
                transform->scale.x = transformComponent.GetArray()[6].GetFloat();
                transform->scale.y = transformComponent.GetArray()[7].GetFloat();
                transform->scale.z = transformComponent.GetArray()[8].GetFloat();
            }

            auto& meshComponent = entityDetails["MeshComponent"];
            if (meshComponent.IsObject() && meshComponent["MeshIndex"].IsUint())
            {
                auto mesh = m_ActiveScene->m_Registry->AddComponent<MeshComponent>(entity);
                mesh->MeshIndex = meshComponent["MeshIndex"].GetUint();
                if (meshComponent["MaterialName"].IsString())
                    mesh->MaterialName = meshComponent["MaterialName"].GetString();
            }
        }

        const rapidjson::Value& materials = document["Materials"];
        for (const auto& materialDetails : materials.GetArray())
        {
            FL_ASSERT(materialDetails.IsObject(), "Invalid Material JSON Object!");
            Material material{
                glm::vec3 {
                    materialDetails["Albedo"].GetArray()[0].GetFloat(),
                    materialDetails["Albedo"].GetArray()[1].GetFloat(),
                    materialDetails["Albedo"].GetArray()[2].GetFloat()
                },
                materialDetails["Roughness"].GetFloat(),
                materialDetails["IsMetal"].GetBool()
            };
            m_ActiveScene->m_SceneData.Materials[materialDetails["Name"].GetString()] = material;
        }

        fclose(fp);
        return true;
    }
}
