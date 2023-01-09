#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>

namespace Flameberry {
    struct TransformComponent
    {
        glm::vec3 translation, rotation, scale;
        TransformComponent(): translation(0.0f), rotation(0.0f), scale(1.0f) {};
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
        TagComponent(const std::string& tag = "Default_Entity"): Tag(tag) {}
    };
}
