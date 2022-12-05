#include "SceneSerializer.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>

#include "Core/Timer.h"

namespace Flameberry {
    SceneSerializer::SceneSerializer(Scene* scene)
        : m_ActiveScene(scene)
    {
    }

    SceneSerializer::~SceneSerializer()
    {
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

        FL_ASSERT(document.IsObject(), "Invalid JSON object!");
        FL_LOG("Document has member Scene: {0}", document["Scene"].GetString());

        m_ActiveScene->m_SceneData.Name = document["Scene"].GetString();

        const rapidjson::Value& entities = document["Entities"];
        for (const auto& entityDetails : entities.GetArray())
        {
            FL_LOG("Entity Details:");
            FL_LOG(entityDetails["entityID"].GetInt());
            FL_LOG(entityDetails["TagComponent"].GetString());
            for (const auto& value : entityDetails["TransformComponent"].GetArray())
            {
                FL_LOG(value.GetFloat());
            }

            auto& entity = m_ActiveScene->m_Registry->CreateEntity();

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
        fclose(fp);
        return true;
    }
}
