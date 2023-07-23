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
        ~Scene();

        explicit Scene(const std::shared_ptr<Scene>& other);
        explicit Scene(const Scene& other);
        const Scene& operator=(const Scene& other) const = delete;

        void OnStartRuntime();
        void OnStopRuntime();

        void OnUpdateRuntime(float delta);
        void RenderScene(const glm::mat4& cameraMatrix);
        void OnViewportResize(const glm::vec2& viewportSize);

        fbentt::entity CreateEntityWithTagAndParent(const std::string& tag, fbentt::entity parent);
        void DestroyEntityTree(fbentt::entity entity);
        void ReparentEntity(fbentt::entity entity, fbentt::entity parent);
        bool IsEntityInHierarchy(fbentt::entity key, fbentt::entity parent);
        fbentt::entity DuplicateEntity(fbentt::entity src);
        fbentt::entity CopyEntityTree(fbentt::entity src);

        fbentt::entity GetPrimaryCameraEntity() const;

        inline std::shared_ptr<fbentt::registry> GetRegistry() const { return m_Registry; }
        inline glm::vec3 GetClearColor() const { return m_Environment.ClearColor; }
        inline std::string GetName() const { return m_Name; }
        inline DirectionalLight GetDirectionalLight() const { return m_Environment.DirLight; }
        void SetDirectionalLight(const DirectionalLight& light) { m_Environment.DirLight = light; }

        template<typename... Args>
        static std::shared_ptr<Scene> Create(Args... args) { return std::make_shared<Scene>(std::forward<Args>(args)...); }
    private:
        std::shared_ptr<fbentt::registry> m_Registry;

        std::string m_Name = "Untitled";
        Flameberry::Environment m_Environment;

        physx::PxScene* m_PxScene;

        glm::vec2 m_ViewportSize = { 1280, 720 };

        friend class SceneHierarchyPanel;
        friend class InspectorPanel;
        friend class EnvironmentSettingsPanel;
        friend class SceneSerializer;
        friend class SceneRenderer;
    };
}
