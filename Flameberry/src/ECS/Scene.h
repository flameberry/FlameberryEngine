#pragma once

#include <unordered_map>
#include <memory>

#include "ecs.hpp"
#include "Renderer/Light.h"
#include "Renderer/Material.h"
#include "Renderer/StaticMesh.h"

namespace physx { class PxScene; }

namespace Flameberry {
    class Scene
    {
    public:
        Scene();
        ~Scene();

        explicit Scene(const Ref<Scene>& other);
        explicit Scene(const Scene& other);
        const Scene& operator=(const Scene& other) const = delete;

        void OnStartRuntime();
        void OnStopRuntime();

        void OnUpdateRuntime(float delta);
        void RenderScene(const glm::mat4& cameraMatrix);
        void OnViewportResize(const glm::vec2& viewportSize);

        fbentt::entity CreateEntityWithTagAndParent(const std::string& tag, fbentt::entity parent);
        void DestroyEntityTree(fbentt::entity entity);
        void ReparentEntity(fbentt::entity entity, fbentt::entity destParent);
        bool IsEntityInHierarchy(fbentt::entity key, fbentt::entity parent);
        bool IsEntityRoot(fbentt::entity entity);
        fbentt::entity DuplicateEntity(fbentt::entity src);
        fbentt::entity DuplicateSingleEntity(fbentt::entity src);
        fbentt::entity DuplicateEntityTree(fbentt::entity src);

        inline std::string GetName() const { return m_Name; }
        inline Ref<fbentt::registry> GetRegistry() const { return m_Registry; }
        fbentt::entity GetPrimaryCameraEntity() const;
    private:
        bool Recursive_IsEntityInHierarchy(fbentt::entity key, fbentt::entity parent);
    private:
        Ref<fbentt::registry> m_Registry;
        physx::PxScene* m_PxScene;

        std::string m_Name = "Untitled";
        glm::vec2 m_ViewportSize = { 1280, 720 };

        friend class SceneHierarchyPanel;
        friend class InspectorPanel;
        friend class EnvironmentSettingsPanel;
        friend class SceneSerializer;
        friend class SceneRenderer;
    };
}
