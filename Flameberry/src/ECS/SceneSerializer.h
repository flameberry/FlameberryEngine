#pragma once

#include "Scene.h"
#include <memory>

namespace YAML{
    class Emitter;
}

namespace Flameberry {
    class SceneSerializer
    {
    public:
        SceneSerializer(const std::shared_ptr<Scene>& scene);
        ~SceneSerializer();

        bool SerializeScene(const char* scenePath);
        bool DeserializeScene(const char* scenePath);
    private:
        void SerializeEntity(YAML::Emitter& out, ecs::entity_handle& entity);
    private:
        std::shared_ptr<Scene> m_ActiveScene;
    };
}
