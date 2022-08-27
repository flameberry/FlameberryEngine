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
}
