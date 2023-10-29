#pragma once

#include <memory>
#include <set>

#include "Scene.h"

namespace YAML {
    class Emitter;
}

namespace Flameberry {
    class SceneSerializer
    {
    public:
        static std::shared_ptr<Scene> DeserializeIntoNewScene(const char* path);
        static bool DeserializeIntoExistingScene(const char* path, const std::shared_ptr<Scene>& destScene);
        static void SerializeSceneToFile(const char* path, const std::shared_ptr<Scene>& srcScene);
    private:
        static void SerializeEntity(YAML::Emitter& out, const fbentt::entity& entity, const std::shared_ptr<Scene>& scene, std::set<UUID>& assetUUIDs);
    };
}
