#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>

#include "Core/UUID.h"
#include "ecs.hpp"

namespace Flameberry {
    struct IDComponent
    {
        UUID ID;
        IDComponent(UUID id) : ID(id) {}
        IDComponent() = default;
    };

    struct TransformComponent
    {
        glm::vec3 translation, rotation, scale;
        TransformComponent() : translation(0.0f), rotation(0.0f), scale(1.0f) {};
        glm::mat4 GetTransform() const
        {
            return glm::translate(glm::mat4(1.0f), translation)
                * glm::toMat4(glm::quat(rotation))
                * glm::scale(glm::mat4(1.0f), scale);
        }
    };

    struct SpriteRendererComponent
    {
        std::string TextureFilePath = "";
        glm::vec4 Color{ 1.0f };
    };

    struct TagComponent
    {
        std::string Tag;
        TagComponent(const std::string& tag = "Default_Entity") : Tag(tag) {}
    };

    struct CameraComponent
    {
    };

    struct MeshComponent
    {
        UUID MeshUUID;
        MeshComponent(UUID meshUUID = 0) : MeshUUID(meshUUID) {}
    };

    struct LightComponent
    {
        glm::vec3 Color;
        float Intensity;

        LightComponent() : Color(1.0f), Intensity(10.0f) {}
    };

    struct RelationshipComponent
    {
        fbentt::entity Parent{};
        fbentt::entity FirstChild{};
        fbentt::entity PrevSibling{};
        fbentt::entity NextSibling{};

        RelationshipComponent()
            : Parent(fbentt::null), FirstChild(fbentt::null), PrevSibling(fbentt::null), NextSibling(fbentt::null)
        {
        }

        RelationshipComponent(const RelationshipComponent& dest)
            : Parent(dest.Parent), FirstChild(dest.FirstChild), PrevSibling(dest.PrevSibling), NextSibling(dest.NextSibling)
        {
        }
    };
}
