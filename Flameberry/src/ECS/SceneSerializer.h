#pragma once

#include "Scene.h"

namespace Flameberry {
    class SceneSerializer
    {
    public:
        SceneSerializer(Scene* scene);
        ~SceneSerializer();

        bool DeserializeScene(const char* scenePath);
    private:
        Scene* m_ActiveScene;
    };
}
