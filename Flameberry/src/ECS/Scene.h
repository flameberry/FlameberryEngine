#pragma once

#include <unordered_map>
#include <memory>

#include "ecs.hpp"
#include "Renderer/Light.h"
#include "Renderer/Material.h"
#include "Renderer/Vulkan/StaticMesh.h"

namespace Flameberry {
    struct EnvironmentMap
    {
        glm::vec3 ClearColor;
        // std::shared_ptr<Skybox> ActiveSkybox;
        bool EnableSkybox, Reflections;
        DirectionalLight DirLight;

        EnvironmentMap()
            : ClearColor(0.05f, 0.05f, 0.05f),
            EnableSkybox(false),
            Reflections(false)
        {}
    };

    struct SceneData
    {
        std::string Name = "Untitled";
        EnvironmentMap ActiveEnvironmentMap;
    };

    class Scene
    {
    public:
        Scene(const std::shared_ptr<fbentt::registry>& reg);
        ~Scene() = default;

        void RenderScene(const glm::mat4& cameraMatrix);
        void SetDirectionalLight(const DirectionalLight& light) { m_SceneData.ActiveEnvironmentMap.DirLight = light; }

        inline glm::vec3 GetClearColor() const { return m_SceneData.ActiveEnvironmentMap.ClearColor; }
        inline std::string GetName() const { return m_SceneData.Name; }
        inline DirectionalLight GetDirectionalLight() const { return m_SceneData.ActiveEnvironmentMap.DirLight; }

        template<typename... Args>
        static std::shared_ptr<Scene> Create(Args... args) { return std::make_shared<Scene>(std::forward<Args>(args)...); }
    private:
        std::shared_ptr<fbentt::registry> m_Registry;
        SceneData m_SceneData;

        friend class SceneHierarchyPanel;
        friend class InspectorPanel;
        friend class EnvironmentSettingsPanel;
        friend class SceneSerializer;
        friend class SceneRenderer;
    };
}
