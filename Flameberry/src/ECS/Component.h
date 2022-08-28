#pragma once

#include <glm/glm.hpp>

namespace Flameberry {
    extern uint32_t typeCounter;

    template<class T>
    uint32_t GetComponentTypeId()
    {
        static uint32_t componentCounter = typeCounter++;
        return componentCounter;
    }

    struct TransformComponent { glm::vec3 position, rotation, scale; TransformComponent() = default; };

    struct SpriteRendererComponent
    {
        std::string TextureFilePath;
        glm::vec4 Color{ 1.0f };
    };
}
