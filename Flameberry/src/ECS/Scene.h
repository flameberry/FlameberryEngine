#pragma once

#include <unordered_map>
#include <memory>

#include "ecs.hpp"
#include "Renderer/Light.h"
#include "Renderer/Material.h"
#include "Renderer/StaticMesh.h"

namespace physx { class PxScene; }

namespace Flameberry {
    struct Environment
    {
        glm::vec3 ClearColor;
        std::shared_ptr<Texture2D> EnvironmentMap;
        bool EnableEnvironmentMap, Reflections;
        DirectionalLight DirLight;

        Environment()
            : ClearColor(0.05f, 0.05f, 0.05f), EnableEnvironmentMap(false), Reflections(false)
        {}
    };

    class Scene
    {
    public:
        Scene();
        explicit Scene(const std::shared_ptr<Scene>& other);
        ~Scene();

        void OnStartRuntime();
        void OnStopRuntime();

        void OnUpdateRuntime(float delta);
        void RenderScene(const glm::mat4& cameraMatrix);
        void SetDirectionalLight(const DirectionalLight& light) { m_Environment.DirLight = light; }

        inline std::shared_ptr<fbentt::registry> GetRegistry() const { return m_Registry; }
        inline glm::vec3 GetClearColor() const { return m_Environment.ClearColor; }
        inline std::string GetName() const { return m_Name; }
        inline DirectionalLight GetDirectionalLight() const { return m_Environment.DirLight; }

        template<typename... Args>
        static std::shared_ptr<Scene> Create(Args... args) { return std::make_shared<Scene>(std::forward<Args>(args)...); }
    private:
        std::shared_ptr<fbentt::registry> m_Registry;

        std::string m_Name = "Untitled";
        Flameberry::Environment m_Environment;

        physx::PxScene* m_PxScene;

        friend class SceneHierarchyPanel;
        friend class InspectorPanel;
        friend class EnvironmentSettingsPanel;
        friend class SceneSerializer;
        friend class SceneRenderer;
    };
}
