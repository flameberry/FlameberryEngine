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
        static Ref<Scene> DeserializeIntoNewScene(const char* path);
        static bool DeserializeIntoExistingScene(const char* path, const Ref<Scene>& destScene);
        static void SerializeSceneToFile(const char* path, const Ref<Scene>& srcScene);
    private:
        static void SerializeEntity(YAML::Emitter& out, const fbentt::entity& entity, const Ref<Scene>& scene, std::set<UUID>& assetUUIDs);
    };
}
