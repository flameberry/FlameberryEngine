#pragma once

#include "Registry.h"

namespace Flameberry {
    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;
    private:
        std::shared_ptr<Registry> m_Registry;
    };
}