#pragma once

#include <unordered_map>
#include <memory>

#include "ecs.hpp"
#include "Renderer/Light.h"
#include "Renderer/Material.h"
#include "Renderer/Vulkan/StaticMesh.h"

namespace Flameberry {
    struct Environment
    {
        glm::vec3 ClearColor;
        std::shared_ptr<VulkanTexture> EnvironmentMap;
        bool EnableEnvironmentMap, Reflections;
        DirectionalLight DirLight;

        Environment()
            : ClearColor(0.05f, 0.05f, 0.05f), EnableEnvironmentMap(false), Reflections(false)
        {}
    };

    struct SceneData
    {
        std::string Name = "Untitled";
        Environment ActiveEnvironment;
    };

    class Scene
    {
    public:
        Scene(const std::shared_ptr<fbentt::registry>& reg);
        ~Scene() = default;

        void RenderScene(const glm::mat4& cameraMatrix);
        void SetDirectionalLight(const DirectionalLight& light) { m_SceneData.ActiveEnvironment.DirLight = light; }

        inline glm::vec3 GetClearColor() const { return m_SceneData.ActiveEnvironment.ClearColor; }
        inline std::string GetName() const { return m_SceneData.Name; }
        inline DirectionalLight GetDirectionalLight() const { return m_SceneData.ActiveEnvironment.DirLight; }

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
